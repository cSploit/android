
/*

Hydra Form Module
-----------------

The hydra form can be used to carry out a brute-force attack on simple 
web-based login forms that require username and password variables via 
either a GET or POST request.

The module works similarly to the HTTP basic auth module and will honour
proxy mode (with authenticaion) as well as SSL. The module can be invoked
with the service names of "http-get-form", "http-post-form",
"https-get-form" and "https-post-form".

Here's a couple of examples: -

./hydra -l "<userID>" -P pass.txt 10.221.64.12 http-post-form
"/irmlab2/testsso-auth.do:ID=^USER^&Password=^PASS^:Invalid Password"

./hydra -S -s 443 -l "<username>" -P pass.txt 10.221.64.2 https-get-form 
"/irmlab1/vulnapp.php:username=^USER^&pass=^PASS^:incorrect"

The option field (following the service field) takes three ":" separated
values and an optional fourth value, the first is the page on the server
to GET or POST to, the second is the POST/GET variables (taken from either
the browser, or a proxy such as PAROS) with the varying usernames and passwords
in the "^USER^" and "^PASS^" placeholders, the third is the string that it
checks for an *invalid* or *valid* login - any exception to this is counted
as a success.
So please:
 * invalid condition login should be preceded by "F="
 * valid condition login should be preceded by "S=".
By default, if no header is found the condition is assume to be a fail,
so checking for *invalid* login.
The fourth optional value, can be a 'C' to define a different page to GET
initial cookies from.

If you specify the verbose flag (-v) it will show you the response from the 
HTTP server which is useful for checking the result of a failed login to 
find something to pattern match against.

Module initially written by Phil Robinson, IRM Plc (releases@irmplc.com),
rewritten by David Maciejak

Fix and issue with strtok use and implement 1 step location follow if HTTP
3xx code is returned (david dot maciejak at gmail dot com)

Added fail or success condition, getting cookies, and allow 5 redirections by david

*/

#include "hydra-mod.h"

extern char *HYDRA_EXIT;
char *buf;
char *cond;

int success_cond = 0;
int getcookie = 1;
int auth_flag = 0;

char redirected_url_buff[2048] = "";
int redirected_flag = 0;

#define MAX_REDIRECT 8
int redirected_cpt = MAX_REDIRECT;
char cookie[4096] = "", cmiscptr[1024];

extern char *webtarget;
extern char *slash;
int webport, freemischttpform = 0;
char bufferurl[1024], cookieurl[1024] = "", userheader[1024] = "", *url, *variables, *optional1;

int strpos(char *str, char *target) {
  char *res = strstr(str, target);

  if (res == NULL)
    return -1;
  else
    return res - str;
}

char *html_encode(char *string) {
  char *ret = string;

  if (ret == NULL)
    return NULL;

  if (index(ret, '%') != NULL)
    ret = hydra_strrep(ret, "%", "%25");
  if (index(ret, ' ') != NULL)
    ret = hydra_strrep(ret, " ", "%20");
  if (index(ret, '&') != NULL)
    ret = hydra_strrep(ret, "&", "%26");
  if (index(ret, '#') != NULL)
    ret = hydra_strrep(ret, "&", "%23");

  return ret;
}


/*
int analyze_server_response(int socket)
return 0 or 1 when the cond regex is matched
return -1 if no response from server
*/
int analyze_server_response(int s) {
  int runs = 0;

  while ((buf = hydra_receive_line(s)) != NULL) {
    runs++;
    //check for http redirection
    if (strstr(buf, "HTTP/1.1 3") != NULL || strstr(buf, "HTTP/1.0 3") != NULL || strstr(buf, "Status: 3") != NULL) {
      redirected_flag = 1;
    } else if (strstr(buf, "HTTP/1.1 401") != NULL || strstr(buf, "HTTP/1.0 401") != NULL) {
      auth_flag = 1;
    } else if ((strstr(buf, "HTTP/1.1 403") != NULL) || (strstr(buf, "HTTP/1.1 404") != NULL) || (strstr(buf, "HTTP/1.0 403") != NULL) || (strstr(buf, "HTTP/1.0 404") != NULL)) {
      return 0;
    }

    if (hydra_strcasestr(buf, "Location: ") != NULL) {
      char *startloc, *endloc;
      char str[2048];

      startloc = hydra_strcasestr(buf, "Location: ") + strlen("Location: ");
      strncpy(str, startloc, sizeof(str) - 1);
      str[sizeof(str) - 1] = 0;
      endloc = strchr(str, '\n');
      if (endloc != NULL)
        *endloc = 0;
      endloc = strchr(str, '\r');
      if (endloc != NULL)
        *endloc = 0;
      strcpy(redirected_url_buff, str);
    }
    //there can be multiple cookies
    if (hydra_strcasestr(buf, "Set-Cookie: ") != NULL) {
      char *cookiebuf = buf;

      do {
        char *startcookie, *endcookie1, *endcookie2;
        char str[1024], tmpcookie[4096] = "", tmpname[128] = "", *ptr, *ptr2;

        memset(str, 0, sizeof(str));
        startcookie = hydra_strcasestr(cookiebuf, "Set-Cookie: ") + strlen("Set-Cookie: ");
        strncpy(str, startcookie, sizeof(str) - 1);
        str[sizeof(str) - 1] = 0;
        endcookie1 = strchr(str, '\n');
        endcookie2 = strchr(str, ';');
        //terminate string after cookie data
        if (endcookie1 != NULL && endcookie1 < endcookie2)
          *endcookie1 = 0;
        else if (endcookie2 != NULL)
          *endcookie2 = 0;
        // is the cookie already there? if yes, remove it!
        if (index(startcookie, '=') != NULL && (ptr = index(startcookie, '=')) - startcookie + 1 <= sizeof(tmpname)) {
          strncpy(tmpname, startcookie, sizeof(tmpname) - 2);
          tmpname[sizeof(tmpname) - 2] = 0;
          ptr = index(tmpname, '=');
          *(++ptr) = 0;
          // is the cookie already in the cookiejar? (so, does it have to be replaced?)
          if ((ptr = hydra_strcasestr(cookie, tmpname)) != NULL) {
            // yes it is.
            // if the cookie is not in the beginning of the cookiejar, copy the ones before
            if (ptr != cookie && *(ptr - 1) == ' ') {
              strncpy(tmpcookie, cookie, ptr - cookie - 2);
              tmpcookie[ptr - cookie - 2] = 0;
            }
            ptr += strlen(tmpname);
            // if there are any cookies after this one in the cookiejar, copy them over
            if ((ptr2 = strstr(ptr, "; ")) != NULL) {
              ptr2 += 2;
              strncat(tmpcookie, ptr2, sizeof(tmpcookie) - strlen(tmpcookie) - 1);
            }
            if (debug)
              printf("[DEBUG] removing cookie %s in jar\n before: %s\n after:  %s\n", tmpname, cookie, tmpcookie);
            strcpy(cookie, tmpcookie);
          }
        }
        ptr = index(str, '=');
        // only copy the cookie if it has a value (otherwise the server wants to delete the cookie
        if (ptr != NULL && *(ptr + 1) != ';' && *(ptr + 1) != 0 && *(ptr + 1) != '\n' && *(ptr + 1) != '\r') {
          if (strlen(cookie) > 0)
            strncat(cookie, "; ", sizeof(cookie) - strlen(cookie) - 1);
          strncat(cookie, str, sizeof(cookie) - strlen(cookie) - 1);
        }
        cookiebuf = startcookie;
      } while (hydra_strcasestr(cookiebuf, "Set-Cookie: ") != NULL);
    }
#ifdef HAVE_PCRE
    if (hydra_string_match(buf, cond) == 1) {
#else
    if (strstr(buf, cond) != NULL) {
#endif
      free(buf);
//      printf("DEBUG: STRING %s FOUND!!:\n%s\n", cond, buf);
      return 1;
    }
//    else printf("DEBUG: STRING %s NOT FOUND:\n%s\n", cond, buf);
    free(buf);
  }
  if (runs == 0) {
    if (debug)
      hydra_report(stderr, "DEBUG: no response from server\n");
    return -1;
  }
  return 0;
}

void hydra_reconnect(int s, char *ip, int port, unsigned char options) {
  if (s >= 0)
    s = hydra_disconnect(s);
  if ((options & OPTION_SSL) == 0) {
    s = hydra_connect_tcp(ip, port);
  } else {
    s = hydra_connect_ssl(ip, port);
  }
}

int start_http_form(int s, char *ip, int port, unsigned char options, char *miscptr, FILE * fp, char *type) {
  char *empty = "";
  char *login, *pass, buffer[9000], clogin[256], cpass[256];
  char header[8096], *upd3variables, cuserheader[1024];
  int found = !success_cond, i, j;

  memset(header, 0, sizeof(header));
  cookie[0] = 0;                // reset cookies from potential previous attempt

  if (strlen(login = hydra_get_next_login()) == 0)
    login = empty;
  if (strlen(pass = hydra_get_next_password()) == 0)
    pass = empty;
  strncpy(clogin, html_encode(login), sizeof(clogin) - 1);
  clogin[sizeof(clogin) - 1] = 0;
  strncpy(cpass, html_encode(pass), sizeof(cpass) - 1);
  cpass[sizeof(cpass) - 1] = 0;
  upd3variables = hydra_strrep(variables, "^USER^", clogin);
  upd3variables = hydra_strrep(upd3variables, "^PASS^", cpass);
  if (strstr(userheader, "^USER^") == NULL && strstr(userheader, "^PASS^") == NULL) {
    strcpy(cuserheader, userheader);
  } else {                      // we use the encoded version
    strncpy(cuserheader, hydra_strrep(userheader, "^USER^", clogin), sizeof(cuserheader) - 1);
    cuserheader[sizeof(cuserheader) - 1] = 0;
    strncpy(cuserheader, hydra_strrep(cuserheader, "^PASS^", cpass), sizeof(cuserheader) - 1);
    cuserheader[sizeof(cuserheader) - 1] = 0;
  }

  /* again: no snprintf to be portable. dont worry, buffer cant overflow */
  if (use_proxy == 1 && proxy_authentication != NULL) {
    // proxy with authentication
    if (getcookie) {
      //doing a GET to save cookies
      sprintf(buffer, "GET http://%s:%d%.600s HTTP/1.0\r\nHost: %s\r\nProxy-Authorization: Basic %s\r\nUser-Agent: Mozilla 5.0 (Hydra Proxy Auth)\r\n%s%s\r\n",
              webtarget, webport, cookieurl, webtarget, proxy_authentication, header, cuserheader);
      if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
        return 1;
      }
      i = analyze_server_response(s);   // return value ignored
      if (strlen(cookie) > 0) {
        sprintf(header, "Cookie: %s\r\n", cookie);
      }
      hydra_reconnect(s, ip, port, options);
    }

    if (strcmp(type, "POST") == 0) {
      sprintf(buffer,
              "POST http://%s:%d%.600s HTTP/1.0\r\nHost: %s\r\nProxy-Authorization: Basic %s\r\nUser-Agent: Mozilla/5.0 (Hydra Proxy Auth)\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n%s%s\r\n%s",
              webtarget, webport, url, webtarget, proxy_authentication, (int) strlen(upd3variables), header, cuserheader, upd3variables);
      if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
        return 1;
      }
    } else {
      sprintf(buffer,
              "GET http://%s:%d%.600s?%s HTTP/1.0\r\nHost: %s\r\nProxy-Authorization: Basic %s\r\nUser-Agent: Mozilla/5.0 (Hydra Proxy Auth)\r\n%s%s\r\n",
              webtarget, webport, url, upd3variables, webtarget, proxy_authentication, header, cuserheader);
      if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
        return 1;
      }
    }
  } else {
    if (use_proxy == 1) {
      // proxy without authentication
      if (getcookie) {
        //doing a GET to get cookies
        sprintf(buffer, "GET http://%s:%d%.600s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Hydra Proxy)\r\n%s%s\r\n", webtarget, webport, cookieurl, webtarget, header,
                cuserheader);
        if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
          return 1;
        }
        i = analyze_server_response(s); // ignore result
        if (strlen(cookie) > 0) {
          sprintf(header, "Cookie: %s\r\n", cookie);
        }
        hydra_reconnect(s, ip, port, options);
      }

      if (strcmp(type, "POST") == 0) {
        sprintf(buffer,
                "POST http://%s:%d%.600s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Hydra)\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n%s%s\r\n%s",
                webtarget, webport, url, webtarget, (int) strlen(upd3variables), header, cuserheader, upd3variables);
        if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
          return 1;
        }
      } else {
        sprintf(buffer, "GET http://%s:%d%.600s?%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Hydra)\r\n%s%s\r\n", webtarget, webport, url, upd3variables, webtarget,
                header, cuserheader);
        if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
          return 1;
        }
      }
    } else {
      // direct web server, no proxy
      if (getcookie) {
        //doing a GET to save cookies
        sprintf(buffer, "GET %.600s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Hydra)\r\n%s\r\n", cookieurl, webtarget, cuserheader);
        if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
          return 1;
        }
        i = analyze_server_response(s); // ignore result
        if (strlen(cookie) > 0) {
          sprintf(header, "Cookie: %s\r\n", cookie);
        }
        hydra_reconnect(s, ip, port, options);
      }

      if (strcmp(type, "POST") == 0) {
        sprintf(buffer,
                "POST %.600s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Hydra)\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n%s%s\r\n%s",
                url, webtarget, (int) strlen(upd3variables), header, cuserheader, upd3variables);
        if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
          return 1;
        }
      } else {
        sprintf(buffer, "GET %.600s?%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0 (Hydra)\r\n%s%s\r\n", url, upd3variables, webtarget, header, cuserheader);
        if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
          return 1;
        }
      }
    }
  }

  found = analyze_server_response(s);
  if (auth_flag) {              // we received a 401 error - user using wrong module
    hydra_report(stderr, "[ERROR] the target is using HTTP auth, not a web form, received HTTP error code 401. Use module \"http%s-get\" instead.\n",
                 (options & OPTION_SSL) > 0 ? "s" : "");
    return 4;
  }
  if (strlen(cookie) > 0) {
    sprintf(header, "Cookie: %.1000s\r\n", cookie);
  }
  //if page was redirected, follow the location header
  redirected_cpt = MAX_REDIRECT;
  if (debug)
    printf("[DEBUG] attempt result: found %d, redirect %d, location: %s\n", found, redirected_flag, redirected_url_buff);
  while (found == 0 && redirected_flag && (redirected_url_buff[0] != 0) && (redirected_cpt > 0)) {
    //we have to split the location
    char *startloc, *endloc;
    char str[2048];
    char str2[2048];
    char str3[2048];

    redirected_cpt--;
    redirected_flag = 0;
    //check if the redirect page contains the fail/success condition
#ifdef HAVE_PCRE
    if (hydra_string_match(redirected_url_buff, cond) == 1) {
#else
    if (strstr(redirected_url_buff, cond) != NULL) {
#endif
      found = success_cond;
    } else {
      //location could be either absolute http(s):// or / something
      //or relative
      startloc = strstr(redirected_url_buff, "://");
      if (startloc != NULL) {
        startloc += strlen("://");

        if ((endloc = strchr(startloc, '\r')) != NULL) {
          startloc[endloc - startloc] = 0;
        }
        if ((endloc = strchr(startloc, '\n')) != NULL) {
          startloc[endloc - startloc] = 0;
        }
        strncpy(str, startloc, sizeof(str));
        str[sizeof(str) - 1] = 0;

        endloc = strchr(str, '/');
        if (endloc != NULL) {
          strncpy(str2, str, endloc - str);
          str2[endloc - str] = 0;
        } else
          strncpy(str2, str, sizeof(str));

        if (strlen(str) - strlen(str2) == 0) {
          strcpy(str3, "/");
        } else {
          strncpy(str3, str + strlen(str2), strlen(str) - strlen(str2));
          str3[strlen(str) - strlen(str2)] = 0;
        }
      } else {
        strncpy(str2, webtarget, sizeof(str2) - 1);
        str2[sizeof(str2) - 1] = 0;
        if (redirected_url_buff[0] != '/') {
          //it's a relative path, so we have to concatenate it
          //with the path from the first url given
          char *urlpath;
          char urlpath_extracted[2048];

          memset(urlpath_extracted, 0, sizeof(urlpath_extracted));

          urlpath = strrchr(url, '/');
          if (urlpath != NULL) {
            strncpy(urlpath_extracted, url, urlpath - url);
            sprintf(str3, "%.1000s/%.1000s", urlpath_extracted, redirected_url_buff);
          } else {
            sprintf(str3, "%.1000s/%.1000s", url, redirected_url_buff);
          }
        } else
          strncpy(str3, redirected_url_buff, sizeof(str3));
        if (debug)
          hydra_report(stderr, "[DEBUG] host=%s redirect=%s origin=%s\n", str2, str3, url);
      }
      if (str3[0] != '/') {
        j = strlen(str3);
        str3[j + 1] = 0;
        for (i = j; i > 0; i--)
          str3[i] = str3[i - 1];
        str3[0] = '/';
      }

      if (verbose)
        hydra_report(stderr, "[VERBOSE] Page redirected to http://%s%s\n", str2, str3);

      //re-use the code above to check for proxy use
      if (use_proxy == 1 && proxy_authentication != NULL) {
        // proxy with authentication
        sprintf(buffer, "GET http://%s:%d%.600s HTTP/1.0\r\nHost: %s\r\nProxy-Authorization: Basic %s\r\nUser-Agent: Mozilla/4.0 (Hydra)\r\n%s\r\n",
                webtarget, webport, str3, str2, proxy_authentication, header);
      } else {
        if (use_proxy == 1) {
          // proxy without authentication
          sprintf(buffer, "GET http://%s:%d%.600s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/4.0 (Hydra)\r\n%s\r\n", webtarget, webport, str3, str2, header);
        } else {
          //direct web server, no proxy
          sprintf(buffer, "GET %.600s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/4.0 (Hydra)\r\n%s\r\n", str3, str2, header);
        }
      }

      hydra_reconnect(s, ip, port, options);

      if (hydra_send(s, buffer, strlen(buffer), 0) < 0) {
        return 1;
      }
      found = analyze_server_response(s);
      if (strlen(cookie) > 0) {
        sprintf(header, "Cookie: %s\r\n", cookie);
      }
    }
  }

  //if the last status is still 3xx, set it as a false
  if (found != -1 && found == success_cond && (redirected_flag == 0 || success_cond == 1) && redirected_cpt >= 0) {
    hydra_report_found_host(port, ip, "www-form", fp);
    hydra_completed_pair_found();
  } else {
    hydra_completed_pair();
  }
  return 1;
}

void service_http_form(char *ip, int sp, unsigned char options, char *miscptr, FILE * fp, int port, char *type) {
  int run = 1, next_run = 1, sock = -1;
  int myport = PORT_HTTP, mysslport = PORT_HTTP_SSL;
  char *ptr, *ptr2;

  hydra_register_socket(sp);

  if (webtarget != NULL && (webtarget = strstr(miscptr, "://")) != NULL) {
    webtarget += strlen("://");
    if ((ptr2 = index(webtarget, ':')) != NULL) {       /* step over port if present */
      *ptr2 = 0;
      ptr2++;
      ptr = ptr2;
      if (*ptr == '/' || (ptr = index(ptr2, '/')) != NULL)
        miscptr = ptr;
      else
        miscptr = slash;        /* to make things easier to user */
    } else if ((ptr2 = index(webtarget, '/')) != NULL) {
      if (freemischttpform == 0) {
        freemischttpform = 1;
        miscptr = malloc(strlen(ptr2) + 1);
        strcpy(miscptr, ptr2);
        *ptr2 = 0;
      }
    } else
      webtarget = NULL;
  }
  if (cmdlinetarget != NULL && webtarget == NULL)
    webtarget = cmdlinetarget;
  else if (webtarget == NULL && cmdlinetarget == NULL)
    webtarget = hydra_address2string(ip);
  if (port != 0)
    webport = port;
  else if ((options & OPTION_SSL) == 0)
    webport = myport;
  else
    webport = mysslport;

  sprintf(bufferurl, "%.1000s", miscptr);
  url = bufferurl;
  ptr = url;
  while (*ptr != 0 && (*ptr != ':' || *(ptr - 1) == '\\'))
    ptr++;
  if (*ptr != 0)
    *ptr++ = 0;
  variables = ptr;
  while (*ptr != 0 && (*ptr != ':' || *(ptr - 1) == '\\'))
    ptr++;
  if (*ptr != 0)
    *ptr++ = 0;
  cond = ptr;
  while (*ptr != 0 && (*ptr != ':' || *(ptr - 1) == '\\'))
    ptr++;
  if (*ptr != 0)
    *ptr++ = 0;
  optional1 = ptr;
  if (strstr(url, "\\:") != NULL) {
    if ((ptr = malloc(strlen(url))) != NULL) { // no need for +1
      strcpy(ptr, hydra_strrep(url, "\\:", ":"));
      url = ptr;
    }
  }
  if (strstr(variables, "\\:") != NULL) {
    if ((ptr = malloc(strlen(variables))) != NULL) { // no need for +1
      strcpy(ptr, hydra_strrep(variables, "\\:", ":"));
      variables = ptr;
    }
  }
  if (strstr(cond, "\\:") != NULL) {
    if ((ptr = malloc(strlen(cond))) != NULL) { // no need for +1
      strcpy(ptr, hydra_strrep(cond, "\\:", ":")); 
      cond = ptr;
    }
  }
  if (url == NULL || variables == NULL || cond == NULL /*|| optional1 == NULL */ )
    hydra_child_exit(2);

//printf("url: %s, var: %s, cond: %s, opt: %s\n", url, variables, cond, optional1);

  if (*cond == 0) {
    fprintf(stderr, "[ERROR] invalid number of parameters in module option\n");
    hydra_child_exit(2);
  }

  sprintf(cookieurl, "%.1000s", url);

  //conditions now have to contain F or S to set the fail or success condition
  if (*cond != 0 && (strpos(cond, "F=") == 0)) {
    success_cond = 0;
    cond += 2;
  } else if (*cond != 0 && (strpos(cond, "S=") == 0)) {
    success_cond = 1;
    cond += 2;
  } else {
    //by default condition is a fail
    success_cond = 0;
  }

  while ( /*(optional1 = strtok(NULL, ":")) != NULL */ *optional1 != 0) {
    switch (optional1[0]) {
    case 'c':                  // fall through
    case 'C':
      ptr = optional1 + 2;
      while (*ptr != 0 && (*ptr != ':' || *(ptr - 1) == '\\'))
        ptr++;
      if (*ptr != 0)
        *ptr++ = 0;
      sprintf(cookieurl, "%.1000s", hydra_strrep(optional1 + 2, "\\:", ":"));
      optional1 = ptr;
      break;
    case 'h':                  // fall through
    case 'H':
      ptr = optional1 + 2;
      while (*ptr != 0 && (*ptr != ':' || *(ptr - 1) == '\\'))
        ptr++;
      if (*ptr != 0)
        *ptr++ = 0;
      ptr2 = ptr;
      while (*ptr2 != 0 && (*ptr2 != ':' || *(ptr2 - 1) == '\\'))
        ptr2++;
      if (*ptr2 != 0)
        *ptr2++ = 0;
      if (sizeof(userheader) - strlen(userheader) > 4) {
        strncat(userheader, optional1 + 2, sizeof(userheader) - strlen(userheader) - 4);
        strcat(userheader, ":");
        strncat(userheader, hydra_strrep(ptr, "\\:", ":"), sizeof(userheader) - strlen(userheader) - 3);
        strcat(userheader, "\r\n");
      }
      optional1 = ptr2;
      break;
      // no default
    }
  }

  while (1) {
    if (run == 2) {
      if (memcmp(hydra_get_next_pair(), &HYDRA_EXIT, sizeof(HYDRA_EXIT)) == 0) {
        if (freemischttpform)
          free(miscptr);
        freemischttpform = 0;
        hydra_child_exit(1);
      }
    }
    switch (run) {
    case 1:                    /* connect and service init function */
      {
        if (sock >= 0)
          sock = hydra_disconnect(sock);
        if ((options & OPTION_SSL) == 0) {
          if (port != 0)
            myport = port;
          sock = hydra_connect_tcp(ip, myport);
          port = myport;
        } else {
          if (port != 0)
            mysslport = port;
          sock = hydra_connect_ssl(ip, mysslport);
          port = mysslport;
        }
        if (sock < 0) {
          hydra_report(stderr, "[ERROR] Child with pid %d terminating, can not connect\n", (int) getpid());
          if (freemischttpform)
            free(miscptr);
          freemischttpform = 0;
          hydra_child_exit(1);
        }
        next_run = 2;
        break;
      }
    case 2:                    /* run the cracking function */
      next_run = start_http_form(sock, ip, port, options, miscptr, fp, type);
      break;
    case 3:                    /* clean exit */
      if (sock >= 0)
        sock = hydra_disconnect(sock);
      if (freemischttpform)
        free(miscptr);
      freemischttpform = 0;
      hydra_child_exit(0);
      break;
    case 4:                    /* silent error exit */
      if (sock >= 0)
        sock = hydra_disconnect(sock);
      if (freemischttpform)
        free(miscptr);
      freemischttpform = 0;
      hydra_child_exit(1);
      break;
    default:
      if (freemischttpform)
        free(miscptr);
      freemischttpform = 0;
      hydra_report(stderr, "[ERROR] Caught unknown return code, exiting!\n");
      hydra_child_exit(0);
    }
    run = next_run;
  }
  if (freemischttpform)
    free(miscptr);
}

void service_http_get_form(char *ip, int sp, unsigned char options, char *miscptr, FILE * fp, int port) {
  service_http_form(ip, sp, options, miscptr, fp, port, "GET");
}

void service_http_post_form(char *ip, int sp, unsigned char options, char *miscptr, FILE * fp, int port) {
  service_http_form(ip, sp, options, miscptr, fp, port, "POST");
}

int service_http_form_init(char *ip, int sp, unsigned char options, char *miscptr, FILE * fp, int port) {
  // called before the childrens are forked off, so this is the function
  // which should be filled if initial connections and service setup has to be
  // performed once only.
  //
  // fill if needed.
  // 
  // return codes:
  //   0 all OK
  //   -1  error, hydra will exit, so print a good error message here

  return 0;
}
