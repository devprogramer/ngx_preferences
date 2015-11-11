#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>



ngx_int_t preference_process(
        ngx_buf_t *data,
        ngx_pool_t    *pool,
        ngx_buf_t  *buf,
        ngx_chain_t   *out,
        size_t *out_size,
        ngx_log_t *log
);
    #ifdef __cplusplus
}
#endif
					
