#ifndef TASK_DMA_UART_ACCESSORY_IOB_H_
#define TASK_DMA_UART_ACCESSORY_IOB_H_

/**
 * @brief Initializes the DMA UART Accessory IOB peripheral and registers the 
 * asynchronous callback handlers.
 * * @return 0 on success, negative error code on failure.
 */
int dma_uart_accessory_iob_init(void);

/**
 * @brief Executes a single cycle of the DMA UART Accessory IOB task loop.
 * Stages data and kicks off the non-blocking hardware transmission.
 */
void dma_uart_accessory_iob_run(void);

#endif /* TASK_DMA_UART_ACCESSORY_IOB_H_ */