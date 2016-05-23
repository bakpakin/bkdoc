#ifndef BKD_HTML_
#define BKD_HTML_

#include "bkd.h"

int bkd_html(struct bkd_ostream * out, struct bkd_document * document);

int bkd_html_fragment(struct bkd_ostream * out, struct bkd_node * node);

#endif /* end of include guard: BKD_HTML_ */
