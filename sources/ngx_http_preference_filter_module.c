#include "ngx_http_preference_filter_module.h"
#include "preference_process.h"


typedef struct {
        ngx_flag_t  enable;
        size_t      buffer_size;
} ngx_http_preference_conf_t;

typedef struct {
    ngx_buf_t           *data;
    unsigned            output:1;
//    ngx_chain_t              *in;

} ngx_http_preference_ctx_t;



static void *ngx_http_preference_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_preference_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);


static ngx_command_t  ngx_http_preference_filter_commands[] = {
        {
                ngx_string("preference"),
                NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
                ngx_conf_set_flag_slot, /*Наверное это и есть колбэк функция для обработки директивы*/
//		ngx_http_preference_set,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_preference_conf_t, enable),
                NULL
        },
        {
            ngx_string("preference_data_buffer"),
            NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
            ngx_conf_set_size_slot,
            NGX_HTTP_LOC_CONF_OFFSET,
            offsetof(ngx_http_preference_conf_t, buffer_size),
            NULL
        },
        ngx_null_command
    };



static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_int_t ngx_http_preference_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_preference_header_filter(ngx_http_request_t *r){

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "preferences   header filter start conf enable recheck");


    ngx_http_preference_ctx_t       *ctx;
    off_t                       len;
    ngx_http_preference_conf_t *conf;

     r->headers_out.status = NGX_HTTP_OK;

    if (r->headers_out.status == NGX_HTTP_NOT_MODIFIED) {
        return ngx_http_next_header_filter(r);
    }

    if (ngx_http_get_module_ctx(r, ngx_http_preference_filter_module)) {
        ngx_http_set_ctx(r, NULL, ngx_http_preference_filter_module);
        return ngx_http_next_header_filter(r);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_preference_filter_module);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "preferences header filter  before conf enable recheck");

    if (!conf->enable) return ngx_http_next_header_filter(r);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "preferences header filter  after conf enable recheck");





    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "preference header filter: Allocating %d bytes for data buffer", len);

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_preference_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }




    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
        "preferences  header filter before ngx_pcalloc");

    len = r->headers_out.content_length_n;
    if (len == -1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "preferences  header filter Missing Content-Length header causes allocating default buffer size");
        //len = conf->buffer_size;
        len = conf->buffer_size;
    }

    ctx->data = ngx_create_temp_buf(r->pool, len);
    if (ctx->data == NULL) return NGX_ERROR;
    ctx->output = 1;
    r->main_filter_need_in_memory = 1;
    ngx_http_set_ctx(r, ctx, ngx_http_preference_filter_module);
    return NGX_OK;


//    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
//        "preference header filter: sizeof %d bytes for data buffer", sizeof(ngx_http_preference_ctx_t));


//    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
//        "preferences  header filter  after ngx_pcalloc");



//    ngx_http_set_ctx(r, ctx, ngx_http_preference_filter_module);
//    ctx->output = 1;







//ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
//                   "http preference header filter header filter in \"%V?%V\"", &r->uri, &r->args);
///* <my test> */
//    r->headers_out.status = NGX_HTTP_OK;
//    return ngx_http_next_header_filter(r);
///* </my test> */

//    time_t  if_modified_since;

//    if_modified_since = ngx_http_parse_time(r->headers_in.if_modified_since->value.data,
//    r->headers_in.if_modified_since->value.len);

//    /* step 1: decide whether to operate */
//    if (if_modified_since != NGX_ERROR &&
//        if_modified_since == r->headers_out.last_modified_time) {

//  /* step 2: operate on the header */
//        r->headers_out.status = NGX_HTTP_NOT_MODIFIED;
//        r->headers_out.content_type.len = 0;
//        ngx_http_clear_content_length(r);
//        ngx_http_clear_accept_ranges(r);
//    }
//    /* step 3: call the next filter */
//  //  return NGX_OK;
//    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_preference_fillbuffer(ngx_buf_t *buf, ngx_chain_t **in);
static ngx_int_t ngx_http_preference_fillbuffer(ngx_buf_t *buf, ngx_chain_t **in)
{
    ngx_chain_t         *cl;
    ngx_buf_t           *b;
    u_char              *p;
    size_t               size, rest;

    p = buf->last;
    rest = buf->end - p;
    cl = *in;

    do {
        b = cl->buf;

        size = b->last - b->pos;
        size = (rest < size) ? rest : size;

        ngx_memcpy(p, b->pos, size);
        rest = rest - size;

        if (!rest) {
            buf->last = buf->end;

            b->pos = b->pos + size;
            if (b->last == b->pos) {
                if (b->last_buf || b->last_in_chain) return NGX_DONE;
                *in = cl->next;
            } else {
                *in = cl;
            }

            return NGX_OK;
        }
        b->pos = b->last;
        p = p + size;
        cl = cl->next;
    } while (cl);

    *in = NULL;
    buf->last = p;

    if (b->last_buf || b->last_in_chain) return NGX_DONE;

    return NGX_AGAIN;
}

static ngx_str_t  ngx_http_sample_text_2 = ngx_string( "I'm so powerful host:     <!--#echo var=\"host\"--> on stage that I seem to have created a monster.\n When I'm performing I'm an extrovert, yet inside I'm a completely different man.");




static ngx_int_t ngx_http_preference_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

static ngx_int_t ngx_http_preference_body_filter(ngx_http_request_t *r, ngx_chain_t *in){


    ngx_http_preference_ctx_t       *ctx;
    ngx_buf_t                  *b;
    if (in == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_preference_filter_module);
    if (ctx == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    ngx_http_preference_conf_t *conf;
    conf = ngx_http_get_module_loc_conf(r, ngx_http_preference_filter_module);
    if (conf->enable) {
        ctx->output = 1;
     }
    if(!ctx->output) return ngx_http_next_body_filter(r, in);



    switch ( ngx_http_preference_fillbuffer(ctx->data, &in)) {
        case NGX_AGAIN:
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  switch NGX_AGAIN");
            return NGX_OK;
        case NGX_DONE:
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  switch NGX_DONE");
        break;
        default:
            if (in == NULL) return NGX_OK;
            b = in->buf;
            if ((b->last_buf || b->last_in_chain) && b->pos == b->last) {
                break;
            }
            return ngx_http_filter_finalize_request(r, &ngx_http_preference_filter_module,NGX_HTTP_INTERNAL_SERVER_ERROR);
    }

    FILE *h;
    h = fopen("/tmp/ng_deb.txt", "a");
    fprintf(h, "\n\n\n\n====================  1 --- %u ---  ==============================\n", ctx->output);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter after declare file");


//    ngx_chain_t *chain_link;


ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  after chain_contains_last_buffer");



//    chain_link = in;
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  after chain_link=in");

    fprintf(h, "\n poss->%s\n last->%s\n start->%s\n end->%s\n"
            "file_pos->%jd file_last->%jd\n"
            "last_buf->%u",
                ctx->data->pos,
            ctx->data->last,
            ctx->data->start,
            ctx->data->end,
            (intmax_t)ctx->data->file_pos,
            (intmax_t)ctx->data->file_last,
            ctx->data->last_buf
            );

 ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  after for cycle");



    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  before callock");

    ngx_http_set_ctx(r, NULL, ngx_http_preference_filter_module);


    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  before send headers");

    ngx_int_t      rc;
    ngx_buf_t    *buf;
    ngx_chain_t   out;
    size_t        out_size;


    buf = ngx_calloc_buf(r->pool);
    if (buf == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  before seting out buf");

    preference_process(ctx->data, r->pool, buf,  &out, &out_size, r->connection->log);


     ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  after process");

//    out.buf = buf;
//    out.next = NULL;
//    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  after seting out buf");
//    buf->start = buf->pos = ngx_http_sample_text.data;
//    buf->end = buf->last = ngx_http_sample_text.data + ngx_http_sample_text.len;
//    buf->memory = 1;
//    buf->last_buf = 1;
//    buf->last_in_chain = 1;











    fprintf(h, "\n\n\n\n===================   2   ===== zu  ================  %s =======\n", out.buf->pos);
    fclose(h);



    if (r == r->main) {
        ngx_http_clear_accept_ranges(r);
        r->headers_out.status = NGX_HTTP_OK;
        r->headers_out.content_length_n = ngx_http_sample_text_2.len;
        r->headers_out.content_length_n = out_size;
      //  r->headers_out.content_type =  ngx_string("");
        if (r->headers_out.content_length) {
            r->headers_out.content_length->hash = 0;
            r->headers_out.content_length = NULL;
        }
    }

    rc = ngx_http_next_header_filter(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) return NGX_ERROR;

    if (&out) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  before send next body filter");
        rc = ngx_http_next_body_filter(r, &out);
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  after send next body filter");
        if (rc == NGX_ERROR){
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,        "preferences  body filter  rc == NGX_ERROR");
             return rc;
        }

    }

    return ngx_http_send_special(r, NGX_HTTP_LAST);
}





static ngx_int_t ngx_http_preference_filter_init(ngx_conf_t *cf);

static ngx_int_t ngx_http_preference_filter_init(ngx_conf_t *cf){
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_preference_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_preference_body_filter;


return NGX_OK;


}




static ngx_http_module_t  ngx_http_preference_filter_module_ctx = {
        NULL,                                  /* preconfiguration */
        ngx_http_preference_filter_init,           /* postconfiguration */
        NULL,       /* create main configuration */
        NULL,    /* init main configuration */
        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        ngx_http_preference_create_loc_conf,        /* create location configuration */
        ngx_http_preference_merge_loc_conf          /* merge location configuration */

};

static void *ngx_http_preference_create_loc_conf(ngx_conf_t *cf){


        ngx_http_preference_conf_t  *conf;
        conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_preference_conf_t));
        if (conf == NULL) {
                return NULL;
        }
        conf->enable = NGX_CONF_UNSET;
        conf->buffer_size = NGX_CONF_UNSET_SIZE;

  //      conf->buffer_size = NGX_CONF_UNSET_SIZE;
  //      conf->tmpls_check = NGX_CONF_UNSET;
        return conf;
}


static char *ngx_http_preference_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
        ngx_http_preference_conf_t *prev = parent;
        ngx_http_preference_conf_t *conf = child;


        ngx_conf_merge_value(conf->enable, prev->enable, 0);
        ngx_conf_merge_size_value(conf->buffer_size, prev->buffer_size, 16 * 1024);

        return NGX_CONF_OK;
}



ngx_module_t  ngx_http_preference_filter_module = {
        NGX_MODULE_V1,
        &ngx_http_preference_filter_module_ctx,     /* module context */
        ngx_http_preference_filter_commands,        /* module directives */
        NGX_HTTP_MODULE,                       /* module type */
        NULL,                                  /* init master */
        NULL,                                  /* init module */
        NULL,                                  /* init process */
        NULL,                                  /* init thread */
        NULL,                                  /* exit thread */
        NULL,                                  /* exit process */
        NULL,                                  /* exit master */
        NGX_MODULE_V1_PADDING
};
