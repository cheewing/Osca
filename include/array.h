#ifndef _SW_ARRAY_H_
#define _SW_ARRAY_H_

#define SW_ARRAY_PAGE_MAX       1024

typedef struct _swArray
{
    void **pages;
    uint16_t page_num;
    uint16_t page_size;
    uint32_t item_size;
    uint32_t item_num;
    uint32_t offset;
}swArray;

#define swArray_page(array, n)      ((n) / (array)->page_size)
#define swArray_offset(array, n)    ((n) % (array)->page_size)

swArray *swArray_new(int page_size, size_t item_size);
void swArray_free(swArray *array);
void *swArray_fetch(swArray *array, uint32_t n);
int swArray_store(swArray *array, uint32_t n, void *data);
void *swArray_alloc(swArray *array, uint32_t n);
int swArray_append(swArray *array, void *data);
int swArray_extend(swArray *array);
void swArray_clear(swArray *array);

#endif /* _SW_ARRAY_H_ */