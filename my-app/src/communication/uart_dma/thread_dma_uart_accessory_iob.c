#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <string.h>
#include "communication/framer.h"
#include "communication/parser.h"

LOG_MODULE_REGISTER(dma_uart_accessory_iob, LOG_LEVEL_DBG);

/* Devicetree node configuration for the target UART peripheral */
#define UART_DMA_NODE DT_NODELABEL(uart4) 
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DMA_NODE);

static uint8_t accessory_iob_tx_buffer[128] __nocache;
static atomic_t tx_busy = ATOMIC_INIT(0);

/* RX Configuration: Double-buffering prevents data loss during processing */
#define RX_BUF_SIZE 64
static uint8_t rx_buf_1[RX_BUF_SIZE] __aligned(32) __nocache;
static uint8_t rx_buf_2[RX_BUF_SIZE] __aligned(32) __nocache;

static uint8_t *next_rx_buf = rx_buf_2;
static bool rx_active = false;

/* Queue to pass received chunks from ISR context to thread context */
struct rx_chunk_msg {
    uint8_t data[RX_BUF_SIZE];
    size_t len;
};
K_MSGQ_DEFINE(accessory_rx_msgq, sizeof(struct rx_chunk_msg), 10, 4);

/* The global framer instance */
static fd_framer_t framer;

/* Callback triggered by the framer when a valid packet is assembled */
static void on_valid_packet_received(const fd_packet_t *pkt)
{
    LOG_DBG("Valid packet framed. Routing to parser...");
    fd_parser_process_packet(INTERFACE_ACCESSORY_IOB, pkt);
}

/* Async UART Callback to catch DMA events */
static void dma_accessory_iob_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
    int ret;
    switch (evt->type) {
    case UART_TX_DONE:
        LOG_INF("Accessory IOB DMA transaction finished. Transmitted %zu bytes.", evt->data.tx.len);
        atomic_clear(&tx_busy);
        break;

    case UART_TX_ABORTED:
        LOG_ERR("Accessory IOB DMA transaction aborted!");
        atomic_clear(&tx_busy);
        break;

    /* --- RX Event Handling --- */
    case UART_RX_RDY:
        /* Fired when data is received or an idle timeout occurs */
        LOG_DBG("RX Data Ready: Received %zu bytes at offset %zu", 
                evt->data.rx.len, evt->data.rx.offset);
        
        if (evt->data.rx.len > 0) {
            struct rx_chunk_msg msg;
            msg.len = evt->data.rx.len;
            
            /* Prevent buffer overflow in case of unexpected length */
            if (msg.len > sizeof(msg.data)) {
                msg.len = sizeof(msg.data);
            }

            /* Copy data out of the DMA buffer immediately */
            memcpy(msg.data, evt->data.rx.buf + evt->data.rx.offset, msg.len);

            /* Send the data chunk to the thread for processing (K_NO_WAIT is crucial in ISR) */
            if (k_msgq_put(&accessory_rx_msgq, &msg, K_NO_WAIT) != 0) {
                LOG_ERR("RX Message Queue is full! Dropping %zu bytes", msg.len);
            }
        }
        break;

    case UART_RX_BUF_REQUEST:
        rx_active = true;
        LOG_DBG("Requesting buffer: %p", next_rx_buf); 
        ret = uart_rx_buf_rsp(dev, next_rx_buf, RX_BUF_SIZE);
        if (ret < 0) {
            LOG_ERR("Failed to provide next RX buffer: %d", ret);
        } else {
            /* Safely toggle the pointer for the NEXT request event */
            next_rx_buf = (next_rx_buf == rx_buf_1) ? rx_buf_2 : rx_buf_1;
        }
        break;

    case UART_RX_BUF_RELEASED:
        /* Fired when the driver is completely done with a buffer chunk */
        LOG_DBG("Buffer %p released by DMA engine.", (void *)evt->data.rx_buf.buf);
        break;

    case UART_RX_DISABLED:
        LOG_INF("UART RX disabled. Re-arming...");
        next_rx_buf = rx_buf_2; // Reset buffer pointer
        uart_rx_enable(dev, rx_buf_1, RX_BUF_SIZE, 5000);
        break;

    case UART_RX_STOPPED:
        rx_active = false;
        LOG_WRN("RX Stopped: %d", evt->data.rx_stop.reason);
        break;

    default:
        break;
    }
}

/* Initialization Function */
int dma_uart_accessory_iob_init(void)
{
    /* 1. Initialize our protocol framer state machine */
    fd_framer_init(&framer, on_valid_packet_received);

    /* 2. Force the peripheral to a clean state */
    uart_irq_rx_disable(uart_dev);
    uart_irq_tx_disable(uart_dev);

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("Display UART hardware not ready");
        return -1;
    }

    int ret = uart_callback_set(uart_dev, dma_accessory_iob_callback, NULL);
    if (ret < 0) {
        LOG_ERR("Failed to set Asynchronous UART Callback: %d", ret);
        return ret;
    }
    
    ret = uart_rx_enable(uart_dev, rx_buf_1, RX_BUF_SIZE, 5000);
    if (ret < 0) {
        LOG_ERR("Failed to start RX DMA: %d", ret);
    } else {
        LOG_INF("RX DMA started successfully.");
    }

    LOG_INF("DMA UART Accessory IOB context fully initialized (TX & RX).");

    /* TEST MESSAGE: Send a dummy read request to RealTerm */
    /* 4C 43 03 01 04 02 0A 00 */
    uint8_t test_msg[] = {0x4C, 0x43, 0x03, 0x01, 0x04, 0x02, 0x0A, 0x00};
    
    /* Forward declaration so the compiler knows about the function below */
    extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);
    
    fd_protocol_send_accessory_iob_dma(test_msg, sizeof(test_msg));

    return 0;
}

/* Runner / Handler Function */
void dma_uart_accessory_iob_run(void)
{
    struct rx_chunk_msg msg;

    /* Block until new DMA bytes arrive in the queue */
    if (k_msgq_get(&accessory_rx_msgq, &msg, K_FOREVER) == 0) {
        
        /* 
         * Feed bytes into the framer state machine.
         * If a complete frame is formed, the framer will synchronously 
         * fire the `on_valid_packet_received` callback on this thread context.
         */
        fd_framer_feed_bytes(&framer, msg.data, msg.len);
    }

    
}



/* Function to transmit a protocol frame over UART DMA */
void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len)
{
    /* Wait if a transmission is already in progress */
    if (atomic_get(&tx_busy) != 0) {
        LOG_WRN("TX busy, dropping packet!");
        return;
    }

    /* Ensure we don't overflow our TX buffer */
    if (len > sizeof(accessory_iob_tx_buffer)) {
        LOG_ERR("Frame too large for TX buffer!");
        return;
    }

    /* Copy to the non-cached DMA buffer */
    memcpy(accessory_iob_tx_buffer, data, len);
    
    /* Flush D-cache to physical RAM before triggering DMA (required for Zephyr on STM32F7) */
    sys_cache_data_flush_range(accessory_iob_tx_buffer, len);

    atomic_set(&tx_busy, 1);

    int ret = uart_tx(uart_dev, accessory_iob_tx_buffer, len, SYS_FOREVER_US);
    if (ret < 0) {
        LOG_ERR("Failed to initiate DMA TX: %d", ret);
        atomic_clear(&tx_busy);
    }
}