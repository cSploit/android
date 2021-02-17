PHP_ARG_ENABLE(auth-nds,whether to enable NDS authentication,
[  --enable-auth-nds       enable NDS authentication])

if test "$PHP_AUTH_NDS" != "no"; then
  PHP_EXTENSION(auth-nds, $ext_shared)
fi
