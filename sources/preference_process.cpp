#include "preference_process.h"



static ngx_str_t  ngx_http_sample_text = ngx_string( "I'm so powerful host:     <!--#echo var=\"host\"--> on stage that I seem to have created a monster.\n When I'm performing I'm an extrovert, yet inside I'm a completely different man.");

ngx_int_t preference_process(ngx_buf_t *data, ngx_pool_t  *pool,  ngx_buf_t  *buf,   ngx_chain_t   *out,
                             size_t *out_size, ngx_log_t *log){


    ngx_chain_t          _out;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,        "preferences  body filter  before seting out buf in preference process");
    _out.buf = buf;
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,        "preferences  body filter  before seting out->next  null");
    _out.next = NULL;
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,        "preferences  body filter  after seting out buf in preference process");
    buf->start = buf->pos = ngx_http_sample_text.data;
    buf->end = buf->last = ngx_http_sample_text.data + ngx_http_sample_text.len;
    buf->memory = 1;
    buf->last_buf = 1;
    buf->last_in_chain = 1;





    *out_size = ngx_http_sample_text.len;
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,        "preferences  body filter  before seting *out = _out");
    *out = _out;


    return NGX_DONE;
}
