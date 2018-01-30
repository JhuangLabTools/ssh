#include "myssh.h"

static void R_callback(SEXP fun, const char * buf, ssize_t len){
  if(!Rf_isFunction(fun)) return;
  int ok;
  SEXP str = PROTECT(Rf_allocVector(RAWSXP, len));
  memcpy(RAW(str), buf, len);
  SEXP call = PROTECT(Rf_lcons(fun, Rf_lcons(str, R_NilValue)));
  R_tryEval(call, R_GlobalEnv, &ok);
  UNPROTECT(2);
}

/* Set up tunnel to the target host */
SEXP C_ssh_exec(SEXP ptr, SEXP command, SEXP outfun, SEXP errfun){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_channel channel = ssh_channel_new(ssh);
  bail_if(channel == NULL, "ssh_channel_new", ssh);
  bail_if(ssh_channel_open_session(channel), "ssh_channel_open_session", ssh);
  bail_if(ssh_channel_request_exec(channel, CHAR(STRING_ELT(command, 0))), "ssh_channel_request_exec", ssh);

  static char buffer[1024];
  int nbytes;
  for(int stream = 0; stream < 2; stream++){
    while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), stream)) > 0)
      R_callback(stream > 0 ? errfun : outfun, buffer, nbytes);
  }
  int status = ssh_channel_get_exit_status(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
  return Rf_ScalarInteger(status);
}
