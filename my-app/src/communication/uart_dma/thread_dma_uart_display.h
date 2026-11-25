#ifndef TASK_DMA_UART_DISPLAY_H_
#define TASK_DMA_UART_DISPLAY_H_

/**
 * @brief Initializes the DMA UART Display peripheral and registers the 
 * asynchronous callback handlers.
 * * @return 0 on success, negative error code on failure.
 */
int dma_uart_display_init(void);

/**
 * @brief Executes a single cycle of the DMA UART Display task loop.
 * Stages data and kicks off the non-blocking hardware transmission.
 */
void dma_uart_display_run(void);

void dma_uart_tx_thread(void);

#endif /* TASK_DMA_UART_DISPLAY_H_ */