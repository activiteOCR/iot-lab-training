#include <string.h>

#include "board.h"
#include "timex.h"
#include "ztimer.h"

/* Add sx127x radio driver necessary includes here */
#include "sx127x.h"
#include "sx127x_netdev.h"
#include "sx127x_params.h"

/* Add loramac necessary includes here */
#include "net/loramac.h"     /* core loramac definitions */
#include "semtech_loramac.h" /* package API */
#include "hts221.h"
#include "hts221_params.h"

/* Add for RX */
#include <stdio.h>

//#ifdef MODULE_SEMTECH_LORAMAC_RX
#include "thread.h"
#include "msg.h"
//#endif

#include "shell.h"

//extern semtech_loramac_t loramac;

#define LORAMAC_RECV_MSG_QUEUE                   (4U)
static msg_t _loramac_recv_queue[LORAMAC_RECV_MSG_QUEUE];
static char _recv_stack[THREAD_STACKSIZE_DEFAULT];
static char stack[THREAD_STACKSIZE_MAIN];

/* Declare the sx127x radio driver descriptor globally here */
static sx127x_t sx127x;      /* The sx127x radio driver descriptor */

/* Declare the loramac descriptor globally here */
static semtech_loramac_t loramac;  /* The loramac stack descriptor */
static hts221_t hts221;

/* Marseille Device and application parameters required for OTAA activation here */
//static const uint8_t deveui[LORAMAC_DEVEUI_LEN] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x04, 0xCE, 0x3D }; 
//static const uint8_t appeui[LORAMAC_APPEUI_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//static const uint8_t appkey[LORAMAC_APPKEY_LEN] = { 0xE6, 0x87, 0x5C, 0x7B, 0x3B, 0x82, 0xEC, 0x6D, 0x9D, 0x3B, 0xA5, 0x3A, 0xA5, 0x96, 0x3F, 0xB6 };

/* Saclay Device and application parameters required for OTAA activation here */
static const uint8_t deveui[LORAMAC_DEVEUI_LEN] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x04, 0xDC, 0x96 };
static const uint8_t appeui[LORAMAC_APPEUI_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t appkey[LORAMAC_APPKEY_LEN] = { 0xD1, 0xB8, 0x4A, 0xF8, 0x2B, 0x62, 0x5F, 0xEB, 0xA3, 0xFA, 0x13, 0x9E, 0x98, 0x82, 0xAB, 0x5B };



static void *thread_handler(void *arg)
{
    printf("thread1\t\n");
    (void)arg;
    while (1) {
         /* do some measurements */
        uint16_t humidity = 0;
        int16_t temperature = 0;
        if (hts221_read_humidity(&hts221, &humidity) != HTS221_OK) {
            puts("Cannot read humidity!");
        }
        if (hts221_read_temperature(&hts221, &temperature) != HTS221_OK) {
            puts("Cannot read temperature!");
        }

        char message[64];
        sprintf(message, "hum: %d.%d temp: %d.%d",
                (humidity / 10), (humidity % 10),
                (temperature / 10), (temperature % 10));
        printf("Sending message '%s'\n", message);

        /* send the message here */
        if (semtech_loramac_send(&loramac,
                                 (uint8_t *)message, strlen(message)) != SEMTECH_LORAMAC_TX_DONE) {
            printf("Cannot send message '%s'\n", message);
        }
        else {
            printf("Message '%s' sent\n", message);
        }

        /* wait 20 seconds between each message */
        ztimer_sleep(ZTIMER_MSEC, 20 * MS_PER_SEC);
    }
    return NULL;
}

static void *_wait_recv(void *arg)
{
    
    msg_init_queue(_loramac_recv_queue, LORAMAC_RECV_MSG_QUEUE);
    (void)arg;
    while (1) {
        /* blocks until something is received */
       switch (semtech_loramac_recv(&loramac)) {
            case SEMTECH_LORAMAC_RX_DATA:
                loramac.rx_data.payload[loramac.rx_data.payload_len] = 0;
                printf("Data received: %s, port: %d\n",
                (char *)loramac.rx_data.payload, loramac.rx_data.port);
                LED2_ON;
                ztimer_sleep(ZTIMER_MSEC, 10 * MS_PER_SEC);
                LED2_OFF;
                break;

            case SEMTECH_LORAMAC_RX_LINK_CHECK:
                printf("Link check information:\n"
                   "  - Demodulation margin: %d\n"
                   "  - Number of gateways: %d\n",
                   loramac.link_chk.demod_margin,
                   loramac.link_chk.nb_gateways);
                break;

            case SEMTECH_LORAMAC_RX_CONFIRMED:
                puts("Received ACK from network");
                break;

            case SEMTECH_LORAMAC_TX_SCHEDULE:
                puts("The Network Server has pending data");
                break;

            default:
                break;
        }
    }
    return NULL;
}

int main(void)
{
    gpio_init(LED2_PIN, GPIO_OUT);
    //LED2_ON;
    
    if (hts221_init(&hts221, &hts221_params[0]) != HTS221_OK) {
        puts("Sensor initialization failed");
        return 1;
    }

    if (hts221_power_on(&hts221) != HTS221_OK) {
        puts("Sensor initialization power on failed");
        return 1;
    }

    if (hts221_set_rate(&hts221, hts221.p.rate) != HTS221_OK) {
        puts("Sensor continuous mode setup failed");
        return 1;
    }
    /* initialize the radio driver */
    sx127x_setup(&sx127x, &sx127x_params[0], 0);
    loramac.netdev = &sx127x.netdev;
    loramac.netdev->driver = &sx127x_driver;

    /* initialize loramac stack */
    semtech_loramac_init(&loramac);

    /* configure the device parameters */
    semtech_loramac_set_deveui(&loramac, deveui);
    semtech_loramac_set_appeui(&loramac, appeui);
    semtech_loramac_set_appkey(&loramac, appkey);

    /* change datarate to DR5 (SF7/BW125kHz) */
    semtech_loramac_set_dr(&loramac, 5);
    
    /* start the OTAA join procedure */
if (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA) != SEMTECH_LORAMAC_JOIN_SUCCEEDED) {
        puts("Join procedure failed");
        return 1;
    }
    puts("Join procedure succeeded");
    
    thread_create(stack, sizeof(stack),
                  THREAD_PRIORITY_MAIN - 1, 0, thread_handler, NULL, "send thread");

    thread_create(_recv_stack, sizeof(_recv_stack),
                  THREAD_PRIORITY_MAIN - 2, 0, _wait_recv, NULL, "recv thread");
    
    while (1) {}
    return 0; /* should never be reached */
}