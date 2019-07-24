/*
** ** Implementation Help **
** https://www.sqlite.org/loadext.html
** 4. Programming Loadable Extensions
**   A template loadable extension contains the following three elements:
**     1. Use "#include <sqlite3ext.h>" at the top of your source code files instead of "#include <sqlite3.h>".
**     2. Put the macro "SQLITE_EXTENSION_INIT1" on a line by itself right after the "#include <sqlite3ext.h>" line.
*/

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <mecab.h>
#include <stdlib.h>
#include <string.h>

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   Before a new auxiliary function or tokenizer implementation may 
**   be registered with FTS5, an application must obtain a pointer
**   to the "fts5_api" structure. ... The following example code 
**   demonstrates the technique: 
*/

/*
** Return a pointer to the fts5_api pointer for database connection db.
** If an error occurs, return NULL and leave an error in the database 
** handle (accessible using sqlite3_errcode()/errmsg()).
*/
fts5_api *fts5_api_from_db(sqlite3 *db){
  fts5_api *pRet = 0;
  sqlite3_stmt *pStmt = 0;

  /*
  ** ** Implementation Help **
  ** https://www.sqlite.org/c3ref/bind_blob.html
  ** ...
  ** The second argument is the index of the SQL parameter to be
  **  set. The leftmost SQL parameter has an index of 1.
  ** ...
  ** sqlite3_bind_pointer(S,I,P,T,D) → Bind pointer P of type T to
  ** the I-th parameter of prepared statement S. D is an optional
  ** destructor function for P. 
  */
  if( SQLITE_OK==sqlite3_prepare(db, "SELECT fts5(?1)", -1, &pStmt, 0) ){
    sqlite3_bind_pointer(pStmt, 1, (void*)&pRet, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
  }
  sqlite3_finalize(pStmt);
  return pRet;
}

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   7.1. Custom Tokenizers
**     To create a custom tokenizer, an application must implement
**     three functions: a tokenizer constructor (xCreate), 
**     a destructor (xDelete) and a function to do the actual 
**     tokenization (xTokenize). ...
**
**     typedef struct Fts5Tokenizer Fts5Tokenizer;
**     typedef struct fts5_tokenizer fts5_tokenizer;
**
*/

/* 
** Implementation Help
** https://taku910.github.io/mecab/libmecab.html
*/
typedef struct MecabTokenizer {
  fts5_tokenizer base;
  mecab_t *mecab;
  int verbose;
  int stop789;
} MecabTokenizer;

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   7.1. Custom Tokenizers
**     To create a custom tokenizer, an application must implement
**     three functions: a tokenizer constructor (xCreate), 
**     a destructor (xDelete) and a function to do the actual 
**     tokenization (xTokenize). ...
**
**     struct fts5_tokenizer {
**       int (*xCreate)(void*, const char **azArg, int nArg, Fts5Tokenizer **ppOut);
**       ....
**     }
**
**     This function is used to allocate and initialize a tokenizer
**     instance. A tokenizer instance is required to actually
**     tokenize text.
**
**     The first argument passed to this function is a copy of 
**     the (void*) pointer provided by the application when the 
**     fts5_tokenizer object was registered with FTS5 (the third
**     argument to xCreateTokenizer()). The second and third
**     arguments are an array of nul-terminated strings containing
**     the tokenizer arguments, if any, specified following the
**     tokenizer name as part of the CREATE VIRTUAL TABLE statement
**     used to create the FTS5 table.
**
**     The final argument is an output variable. If successful, 
**     (*ppOut) should be set to point to the new tokenizer handle
**     and SQLITE_OK returned. If an error occurs, some value other
**     than SQLITE_OK should be returned. In this case, fts5
**     assumes that the final value of *ppOut is undefined. 
*/
static int mecabCreate(
  void *pContext, 
  const char **azArg, int nArg, 
  Fts5Tokenizer **ppOut
){
  // parse args
  int verbose = 0;    // 0 or 1 or 2
  int stop789 = 0;    // 0:false, 1:true

  for (int i = 0; i < nArg; i++) {
    if (strcmp(azArg[i], "stop789") == 0) {
        stop789 = 1;
    } else if (strcmp(azArg[i], "vv") == 0) {
      if (verbose < 2) {
        verbose = 2;
      }
    } else if (strcmp(azArg[i], "v") == 0) {
      if (verbose < 2) {
        verbose += 1;
      }
    } else {
      if (verbose > 0) {
        printf("ignored unknown option: %s\n", azArg[i]);
      }
    }
  }

#ifdef DEBUG
  if (verbose > 0) { // DEBUG LEVEL 1
    printf("mecabCreate()\n"); 
    printf("nArg: %d\n", nArg);
    if (verbose > 1) { // DEBUG LEVEL 2
      for (int i = 0; i < nArg; i++) {
        printf("  %d: %s\n", i, azArg[i]);
      }
    }
    printf("verbose = %d\n", verbose);
    printf("stop789 = %d\n", stop789);
  }
#endif

  fts5_api *pApi = (fts5_api*)pContext;
  MecabTokenizer *p = 0;
  p = sqlite3_malloc(sizeof(MecabTokenizer));
  if (p == NULL) {
    return SQLITE_NOMEM; 
  }
  memset(p, 0, sizeof(MecabTokenizer));
  
  p->mecab = mecab_new(nArg, (char**)azArg);
  if (p->mecab == NULL) {
    sqlite3_free(p);
    return SQLITE_ERROR; 
  }
  p->verbose = verbose;
  p->stop789 = stop789;

#ifdef DEBUG
  /*
  ** ** Implementation Help **
  ** https://taku910.github.io/mecab/libmecab.html
  */
  // Dictionary info
  if (verbose > 0) { // DEBUG LEVEL 1
    const mecab_dictionary_info_t *d = mecab_dictionary_info(p->mecab);
    for (; d; d = d->next) {
      printf("mecab_dictionary_info()\n");
      printf("  filename: %s\n", d->filename);
      printf("  charset: %s\n", d->charset);
      printf("  size: %d\n", d->size);
      printf("  type: %d\n", d->type);
      printf("  lsize: %d\n", d->lsize);
      printf("  rsize: %d\n", d->rsize);
      printf("  version: %d\n", d->version);
    }
  }
#endif

  *ppOut = (Fts5Tokenizer*)p;
  return SQLITE_OK;
}

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   7.1. Custom Tokenizers
**     To create a custom tokenizer, an application must implement
**     three functions: a tokenizer constructor (xCreate), 
**     a destructor (xDelete) and a function to do the actual 
**     tokenization (xTokenize). ...
**     
**     struct fts5_tokenizer {
**       ....
**       void (*xDelete)(Fts5Tokenizer*);
**       ....
**     }
**
**     This function is invoked to delete a tokenizer handle
**     previously allocated using xCreate(). Fts5 guarantees
**     that this function will be invoked exactly once for each 
**     successful call to xCreate(). 
*/
static void mecabDelete(Fts5Tokenizer *pTokenizer){
  MecabTokenizer *p = (MecabTokenizer*)pTokenizer;
#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("mecabDelete()\n");
  }
#endif
  /* 
  ** Implementation Help
  ** https://taku910.github.io/mecab/libmecab.html
  */
  mecab_destroy(p->mecab);
  p->verbose = 0;
  p->stop789 = 0;
  sqlite3_free(p);
}

/*
** ** Implementation Help **
** https://www.sqlite.org/fts5.html
** 7. Extending FTS5
**   7.1. Custom Tokenizers
**     To create a custom tokenizer, an application must implement
**     three functions: a tokenizer constructor (xCreate), 
**     a destructor (xDelete) and a function to do the actual 
**     tokenization (xTokenize). ...
**
**     
**     struct fts5_tokenizer {
**       ....
**       int (*xTokenize)(Fts5Tokenizer*, 
**           void *pCtx,
**           int flags,            /* Mask of FTS5_TOKENIZE_* flags * /
**           const char *pText, int nText, 
**           int (*xToken)(
**             void *pCtx,         /* Copy of 2nd argument to xTokenize() * /
**             int tflags,         /* Mask of FTS5_TOKEN_* flags * /
**             const char *pToken, /* Pointer to buffer containing token * /
**             int nToken,         /* Size of token in bytes * /
**             int iStart,         /* Byte offset of token within input text * /
**             int iEnd            /* Byte offset of end of token within input text * /
**           )
**       );
**     }
**
**     This function is expected to tokenize the nText byte string
**     indicated by argument pText. pText may or may not be 
**     nul-terminated. The first argument passed to this function
**     is a pointer to an Fts5Tokenizer object returned by an
**     earlier call to xCreate().
**
**     The second argument indicates the reason that FTS5 is
**     requesting tokenization of the supplied text. This is
**     always one of the following four values:
**
**          FTS5_TOKENIZE_DOCUMENT - A document is being inserted
**          into or removed from the FTS table. The tokenizer is
**          being invoked to determine the set of tokens to add to
**          (or delete from) the FTS index.
**
**          FTS5_TOKENIZE_QUERY - A MATCH query is being executed
**          against the FTS index. The tokenizer is being called
**          to tokenize a bareword or quoted string specified as
**          part of the query.
**
**          (FTS5_TOKENIZE_QUERY | FTS5_TOKENIZE_PREFIX) - Same as
**          FTS5_TOKENIZE_QUERY, except that the bareword or quoted
**          string is followed by a "*" character, indicating that
**          the last token returned by the tokenizer will be treated
**          as a token prefix.
**
**          FTS5_TOKENIZE_AUX - The tokenizer is being invoked to
**          satisfy an fts5_api.xTokenize() request made by an
**          auxiliary function. Or an fts5_api.xColumnSize() request
**          made by the same on a columnsize=0 database. 
**
**     For each token in the input string, the supplied callback
**     xToken() must be invoked. The first argument to it should be
**     a copy of the pointer passed as the second argument to
**     xTokenize(). The third and fourth arguments are a pointer to
**     a buffer containing the token text, and the size of the token
**     in bytes. The 4th and 5th arguments are the byte offsets of
**     the first byte of and first byte immediately following the
**     text from which the token is derived within the input.
**
**     The second argument passed to the xToken() callback
**     ("tflags") should normally be set to 0. The exception is if
**     the tokenizer supports synonyms. In this case see the
**     discussion below for details.
**
**     FTS5 assumes the xToken() callback is invoked for each token
**     in the order that they occur within the input text.
**
**     If an xToken() callback returns any value other than
**     SQLITE_OK, then the tokenization should be abandoned and the
**     xTokenize() method should immediately return a copy of the
**     xToken() return value. Or, if the input buffer is exhausted,
**     xTokenize() should return SQLITE_OK. Finally, if an error
**     occurs with the xTokenize() implementation itself, it may
**     abandon the tokenization and return any error code other
**     than SQLITE_OK or SQLITE_DONE. 
*/
static int mecabTokenize(
  Fts5Tokenizer *pTokenizer, 
  void *pCtx,
  int flags,            /* Mask of FTS5_TOKENIZE_* flags */
  const char *pText, int nText, 
  int (*xToken)(
    void *pCtx,         /* Copy of 2nd argument to xTokenize() */
    int tflags,         /* Mask of FTS5_TOKEN_* flags */
    const char *pToken, /* Pointer to buffer containing token */
    int nToken,         /* Size of token in bytes */
    int iStart,         /* Byte offset of token within input text */
    int iEnd            /* Byte offset of end of token within input text */
  )
){
  MecabTokenizer *p = (MecabTokenizer*)pTokenizer;
#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("mecabTokenize()\n");
  }
#endif

  const mecab_node_t *node;
  int nlen;
  char *tmp;
  char *buf;
  int buflen;
  int offset;
  int rc;
  
#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("pText (nText) = %s (%d)\n", pText, nText);
  }
#endif

  /* parse */
  node = mecab_sparse_tonode2(p->mecab, pText, strlen(pText)+1);
  if (node == NULL) {
    return SQLITE_ERROR;
  }
  
  /* initialize */
  nlen = 0;
  #define DEFAULT_BUFFER_LENGTH 256
  buf = malloc(DEFAULT_BUFFER_LENGTH);
  if(buf == NULL){
    return SQLITE_NOMEM;
  }
  buflen = DEFAULT_BUFFER_LENGTH;
  offset = 0;
  rc = SQLITE_OK;

#ifdef DEBUG
  int _node_count = 0;  // for DEBUG
  int _token_count = 0; // for DEBUG
#endif
  
  while (node != NULL) {
    while (node->next != NULL && node->length == 0) {

#ifdef DEBUG
      _node_count += 1;
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("increment _node_count [1]: %s\n", node->feature);
      }
#endif
      offset += node->rlength;
      node = node->next;
    }

#ifdef DEBUG
    if (p->verbose > 1) { // DEBUG LEVEL 2
      // printf("pText (nText) = %s (%d)\n", pText, nText);
      printf("node info\n");
      printf("  feature     = %s\n", node->feature);
      printf("  surface     = %s\n", node->surface);
      printf("  length      = %d\n", node->length);
      printf("  rlength     = %d\n", node->rlength);
      printf("  posid       = %d\n", node->posid);
      printf("  char_type   = %d\n", node->char_type);
      printf("  stat        = %d\n", node->stat);
      printf("--------------\n");
    }
#endif

    nlen = node->length;
    offset += node->rlength - nlen;
    if (nlen > buflen) {
      tmp = (char *)realloc(buf, nlen + 1);
      if(tmp == NULL) {
        rc = SQLITE_NOMEM;
        break;
      }else{
        buf = tmp;
      }
      buf[nlen] = '\0';
      buflen = nlen;
    }
    strncpy(buf, node->surface, nlen);
    buf[nlen] = '\0';

#ifdef DEBUG
    if (p->verbose > 1) { // DEBUG LEVEL 2
      printf("calling xToken()\n");
      printf("  tflags      = 0\n");
      printf("  pToken      = %s\n", buf);
      printf("  nToken      = %d\n", nlen);
      printf("  iStart      = %d\n", offset);
      printf("  iEnd        = %d\n", offset + nlen);
      printf("==============\n");
    }
#endif

#ifdef STOP789
    if (!p->stop789 || node->posid > 9 || node->posid < 7) {
      rc = xToken(pCtx, 0, buf, nlen, offset, offset + nlen);
    #ifdef DEBUG
      _token_count += 1;
    } else {
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("increment _node_count [3]: %s\n", node->feature);
      }
    #endif
    }
#else
    rc = xToken(pCtx, 0, buf, nlen, offset, offset + nlen);
    #ifdef DEBUG
    _token_count += 1;
    #endif
#endif

    if (rc != SQLITE_OK) {
      /*
      ** If an xToken() callback returns any value other than
      ** SQLITE_OK, then the tokenization should be abandoned
      ** and the xTokenize() method should immediately return
      ** a copy of the xToken() return value. 
      */

#ifdef DEBUG
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("break [1]: xToken() rc = %d\n", rc);
      }
#endif

      break;
    }
    offset += node->length;

#ifdef DEBUG
    _node_count += 1;
#endif

    node = node->next;
    if (offset >= nText) {
      rc = SQLITE_OK;  // SQLITE_DONE;

#ifdef DEBUG
      if (p->verbose > 0) { // DEBUG LEVEL 1
        printf("break [2]: offset >= nText\n");
      }
#endif

      break;
    }
  }
  
  /* clean up */
  while (node != NULL) {

#ifdef DEBUG
    _node_count += 1;
    if (p->verbose > 0) { // DEBUG LEVEL 1
      printf("increment _node_count [2]: %s\n", node->feature);
    }
#endif

    node = node->next;
  }
  nlen = 0;
  if (buf) {
    free(buf);
  }
  buflen = 0;
  offset = 0;

#ifdef DEBUG
  if (p->verbose > 0) { // DEBUG LEVEL 1
    printf("_node_count, _token_count = %d, %d\n", _node_count, _token_count);
  }
#endif
  
  return rc;
}

/*
** ** Implementation Help **
** https://www.sqlite.org/loadext.html
** 4. Programming Loadable Extensions
**   A template loadable extension contains the following three
**   elements:
**     3. Add an extension loading entry point routine that looks
**        like something the following: 
*/
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_ftsmecab_init( /* entry point for "fts5_mecab.o" */ 
  sqlite3 *db, 
  char **pzErrMsg, 
  const sqlite3_api_routines *pApi
){
  printf("sqlite3_ftsmecab_init()\n");
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);
  /* insert code to initialize your extension here */

  /*
  ** ** Implementation Help **
  ** https://www.sqlite.org/fts5.html
  ** 7. Extending FTS5
  **   Before a new auxiliary function or tokenizer implementation
  **   may be registered with FTS5, an application must obtain a 
  **   pointer to the "fts5_api" structure.
  */
  fts5_api *pApi_fts5;
  
  pApi_fts5 = fts5_api_from_db(db);
  if( pApi_fts5==0 ){
    *pzErrMsg = sqlite3_mprintf("fts5_api_from_db: %s", sqlite3_errmsg(db));
    return SQLITE_ERROR;
  }
  
  /*
  ** ** Implementation Help **
  ** https://www.sqlite.org/fts5.html
  ** 7. Extending FTS5
  **   ...
  **   The fts5_api structure is defined as follows. It exposes
  **   three methods, one each for registering new auxiliary
  **   functions and tokenizers, and one for retrieving existing
  **   tokenizer. The latter is intended to facilitate the
  **   implementation of "tokenizer wrappers" similar to the
  **    built-in porter tokenizer.
  **
  **   typedef struct fts5_api fts5_api;
  **   struct fts5_api {
  **     int iVersion;                   /* Currently always set to 2 * /
  **   
  **     /* Create a new tokenizer * /
  **     int (*xCreateTokenizer)(
  **       fts5_api *pApi,
  **       const char *zName,
  **       void *pContext,
  **       fts5_tokenizer *pTokenizer,
  **       void (*xDestroy)(void*)
  **     );
  **
  **     /* Find an existing tokenizer * /
  **     int (*xFindTokenizer)(
  **       fts5_api *pApi,
  **       const char *zName,
  **       void **ppContext,
  **       fts5_tokenizer *pTokenizer
  **     );
  **
  **     /* Create a new auxiliary function * /
  **     int (*xCreateFunction)(
  **       fts5_api *pApi,
  **       const char *zName,
  **       void *pContext,
  **       fts5_extension_function xFunction,
  **       void (*xDestroy)(void*)
  **     );
  **   };
  **
  **   ...
  **
  **   7.1. Custom Tokenizers
  **     To create a custom tokenizer, an application must implement 
  **     three functions:
  **       a tokenizer constructor (xCreate), 
  **       a destructor (xDelete) 
  **     and
  **      a function to do the actual tokenization (xTokenize). 
  **     ...
  */
  fts5_tokenizer t;
  
  t.xCreate = mecabCreate;
  t.xDelete = mecabDelete;
  t.xTokenize = mecabTokenize;
  rc = pApi_fts5->xCreateTokenizer(pApi_fts5, "mecab", (void*)pApi_fts5, &t, 0);
  return rc;
}
