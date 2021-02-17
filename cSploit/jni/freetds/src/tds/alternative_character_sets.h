/*
 * Copyright 2003 James K. Lowden <jklowden@schemamania.org>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that redistributions of source 
 * code retain the above copyright notice.  
 */
 
 /* 
  * The purpose of this file is to help look up character set names.  
  * 
  * Any given encoding may be known by several (usually similar) aliases.  
  * For example, a system using ASCII encoding may report the character set as 
  * "ASCII", "US-ASCII", or "ISO646-US", among others.  For details on what your system 
  * uses, you may wish to consult the nl_langinfo(3) manual page.  
  *
  * GNU iconv converts a byte sequence from one encoding to another, but before it can do 
  * so, it must be told which is which.  In the list below, the preferred GNU iconv(3) name 
  * is on the left and an alias is on the right.  It is a simple exercise, left to the reader, 
  * to write a function that uses these data to look up the canonical name when provided 
  * an alias.  
  */

#ifndef _ALTERNATIVE_CHARACTER_SETS_H_
#define _ALTERNATIVE_CHARACTER_SETS_H_

/*
 * $Id: alternative_character_sets.h,v 1.10 2005-02-26 13:08:32 freddy77 Exp $
 */
 
/* 
 * This list is sorted alphabetically, except that the most
 * commonly used character sets are first.
 */
 	 /* ASCII */
	  {         "US-ASCII", "US-ASCII"               }
	, {         "US-ASCII", "ANSI_X3.4-1968"         }
	, {         "US-ASCII", "ANSI_X3.4-1986"         }
	, {         "US-ASCII", "ASCII"                  }
	, {         "US-ASCII", "CP367"                  }
	, {         "US-ASCII", "CSASCII"                }
	, {         "US-ASCII", "IBM367"                 }
	, {         "US-ASCII", "ISO-IR-6"               }
	, {         "US-ASCII", "ISO646-US"              }
	, {         "US-ASCII", "ISO_646.IRV:1991"       }
	, {         "US-ASCII", "US"                     }
	, {         "US-ASCII", "646"                    }	/* NetBSD default */
 	 /* ISO_8859-1 */
	, {       "ISO-8859-1", "ISO-8859-1"             }
	, {       "ISO-8859-1", "CP819"                  }
	, {       "ISO-8859-1", "CSISOLATIN1"            }
	, {       "ISO-8859-1", "IBM819"                 }
	, {       "ISO-8859-1", "ISO-IR-100"             }
	, {       "ISO-8859-1", "ISO8859-1"              }
	, {       "ISO-8859-1", "ISO_8859-1"             }
	, {       "ISO-8859-1", "ISO_8859-1:1987"        }
	, {       "ISO-8859-1", "L1"                     }
	, {       "ISO-8859-1", "LATIN1"                 }
	, {       "ISO-8859-1", "iso81"                  }
	, {       "ISO-8859-1", "iso88591"               }
 	 /* UCS-2 */
	, {            "UCS-2", "UCS-2"                  }
	, {            "UCS-2", "CSUNICODE"              }
	, {            "UCS-2", "ISO-10646-UCS-2"        }
	, {            "UCS-2", "UCS2"                   }
	, {            "UCS-2", "ucs2"                   }
	, {   "UCS-2-INTERNAL", "UCS-2-INTERNAL"         }
	, {    "UCS-2-SWAPPED", "UCS-2-SWAPPED"          }
	, {          "UCS-2BE", "UCS-2BE"                }
	, {          "UCS-2BE", "CSUNICODE11"            }
	, {          "UCS-2BE", "UNICODE-1-1"            }
	, {          "UCS-2BE", "UNICODEBIG"             }
	, {          "UCS-2LE", "UCS-2LE"                }
	, {          "UCS-2LE", "UNICODELITTLE"          }
 	 /* UTF-8 */
	, {            "UTF-8", "UTF-8"                  }
	, {            "UTF-8", "UTF8"                   }
	, {            "UTF-8", "utf8"                   }

 	 /* Basically alphabetical from here */
	, {        "ARMSCII-8", "ARMSCII-8"              }
	, {            "BIG-5", "BIG-5"                  }
	, {            "BIG-5", "BIG-FIVE"               }
	, {            "BIG-5", "BIG5"                   }
	, {            "BIG-5", "BIGFIVE"                }
	, {            "BIG-5", "CN-BIG5"                }
	, {            "BIG-5", "CSBIG5"                 }
	, {            "BIG-5", "big5"                   }
	, {       "BIG5-HKSCS", "BIG5-HKSCS"             }
	, {       "BIG5-HKSCS", "BIG5HKSCS"              }
	, {              "C99", "C99"                    }
	, {          "CHINESE", "CHINESE"                }
	, {          "CHINESE", "CSISO58GB231280"        }
	, {          "CHINESE", "GB_2312-80"             }
	, {          "CHINESE", "ISO-IR-58"              }
	, {          "CHINESE", "hp15CN"                 }
	, {               "CN", "CN"                     }
	, {               "CN", "CSISO57GB1988"          }
	, {               "CN", "GB_1988-80"             }
	, {               "CN", "ISO-IR-57"              }
	, {               "CN", "ISO646-CN"              }
	, {            "CN-GB", "CN-GB"                  }
	, {            "CN-GB", "CSGB2312"               }
	, {            "CN-GB", "EUC-CN"                 }
	, {            "CN-GB", "EUCCN"                  }
	, {            "CN-GB", "GB2312"                 }
	, {   "CN-GB-ISOIR165", "CN-GB-ISOIR165"         }
	, {   "CN-GB-ISOIR165", "ISO-IR-165"             }
	, {           "CP1133", "CP1133"                 }
	, {           "CP1133", "IBM-CP1133"             }
	, {           "CP1250", "CP1250"                 }
	, {           "CP1250", "MS-EE"                  }
	, {           "CP1250", "WINDOWS-1250"           }
	, {           "CP1250", "cp1250"                 }
	, {           "CP1251", "CP1251"                 }
	, {           "CP1251", "MS-CYRL"                }
	, {           "CP1251", "WINDOWS-1251"           }
	, {           "CP1251", "cp1251"                 }
	, {           "CP1252", "CP1252"                 }
	, {           "CP1252", "MS-ANSI"                }
	, {           "CP1252", "WINDOWS-1252"           }
	, {           "CP1252", "cp1252"                 }
	, {           "CP1253", "CP1253"                 }
	, {           "CP1253", "MS-GREEK"               }
	, {           "CP1253", "WINDOWS-1253"           }
	, {           "CP1253", "cp1253"                 }
	, {           "CP1254", "CP1254"                 }
	, {           "CP1254", "MS-TURK"                }
	, {           "CP1254", "WINDOWS-1254"           }
	, {           "CP1254", "cp1254"                 }
	, {           "CP1255", "CP1255"                 }
	, {           "CP1255", "MS-HEBR"                }
	, {           "CP1255", "WINDOWS-1255"           }
	, {           "CP1255", "cp1255"                 }
	, {           "CP1256", "CP1256"                 }
	, {           "CP1256", "MS-ARAB"                }
	, {           "CP1256", "WINDOWS-1256"           }
	, {           "CP1256", "cp1256"                 }
	, {           "CP1257", "CP1257"                 }
	, {           "CP1257", "WINBALTRIM"             }
	, {           "CP1257", "WINDOWS-1257"           }
	, {           "CP1257", "cp1257"                 }
	, {           "CP1258", "CP1258"                 }
	, {           "CP1258", "WINDOWS-1258"           }
	, {           "CP1258", "cp1258"                 }
	, {           "CP1361", "CP1361"                 }
	, {           "CP1361", "JOHAB"                  }
	, {            "CP850", "CP850"                  }
	, {            "CP850", "850"                    }
	, {            "CP850", "CSPC850MULTILINGUAL"    }
	, {            "CP850", "IBM850"                 }
	, {            "CP850", "cp850"                  }
	, {            "CP862", "CP862"                  }
	, {            "CP862", "862"                    }
	, {            "CP862", "CSPC862LATINHEBREW"     }
	, {            "CP862", "IBM862"                 }
	, {            "CP862", "cp862"                  }
	, {            "CP866", "CP866"                  }
	, {            "CP866", "866"                    }
	, {            "CP866", "CSIBM866"               }
	, {            "CP866", "IBM866"                 }
	, {            "CP866", "cp866"                  }
	, {            "CP874", "CP874"                  }
	, {            "CP874", "WINDOWS-874"            }
	, {            "CP874", "cp874"                  }
	, {            "CP932", "CP932"                  }
	, {            "CP936", "CP936"                  }
	, {            "CP936", "GBK"                    }
	, {            "CP949", "CP949"                  }
	, {            "CP949", "UHC"                    }
	, {            "CP950", "CP950"                  }
	, {            "CP437", "CP437"                  }
	, {            "CP437", "cp437"                  }
	, {            "CP437", "IBM437"                 }
	, {           "EUC-JP", "EUC-JP"                 }
	, {           "EUC-JP", "CSEUCPKDFMTJAPANESE"    }
	, {           "EUC-JP", "EUCJP"                  }
	, {           "EUC-JP", "EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE"}
	, {           "EUC-JP", "eucJP"                  }
	, {           "EUC-KR", "EUC-KR"                 }
	, {           "EUC-KR", "CSEUCKR"                }
	, {           "EUC-KR", "EUCKR"                  }
	, {           "EUC-KR", "eucKR"                  }
	, {           "EUC-TW", "CSEUCTW"                }
	, {           "EUC-TW", "EUC-TW"                 }
	, {           "EUC-TW", "EUCTW"                  }
	, {           "EUC-TW", "eucTW"                  }
	, {          "GB18030", "GB18030"                }
	, { "GEORGIAN-ACADEMY", "GEORGIAN-ACADEMY"       }
	, {      "GEORGIAN-PS", "GEORGIAN-PS"            }
	, {               "HZ", "HZ"                     }
	, {               "HZ", "HZ-GB-2312"             }
	, {      "ISO-2022-CN", "ISO-2022-CN"            }
	, {      "ISO-2022-CN", "CSISO2022CN"            }
	, {  "ISO-2022-CN-EXT", "ISO-2022-CN-EXT"        }
	, {      "ISO-2022-JP", "ISO-2022-JP"            }
	, {      "ISO-2022-JP", "CSISO2022JP"            }
	, {    "ISO-2022-JP-1", "ISO-2022-JP-1"          }
	, {    "ISO-2022-JP-2", "ISO-2022-JP-2"          }
	, {    "ISO-2022-JP-2", "CSISO2022JP2"           }
	, {      "ISO-2022-KR", "ISO-2022-KR"            }
	, {      "ISO-2022-KR", "CSISO2022KR"            }
	, {      "ISO-8859-10", "ISO-8859-10"            }
	, {      "ISO-8859-10", "CSISOLATIN6"            }
	, {      "ISO-8859-10", "ISO-IR-157"             }
	, {      "ISO-8859-10", "ISO8859-10"             }
	, {      "ISO-8859-10", "ISO_8859-10"            }
	, {      "ISO-8859-10", "ISO_8859-10:1992"       }
	, {      "ISO-8859-10", "L6"                     }
	, {      "ISO-8859-10", "LATIN6"                 }
	, {      "ISO-8859-13", "ISO-8859-13"            }
	, {      "ISO-8859-13", "ISO-IR-179"             }
	, {      "ISO-8859-13", "ISO_8859-13"            }
	, {      "ISO-8859-13", "L7"                     }
	, {      "ISO-8859-13", "LATIN7"                 }
	, {      "ISO-8859-14", "ISO-8859-14"            }
	, {      "ISO-8859-14", "ISO-CELTIC"             }
	, {      "ISO-8859-14", "ISO-IR-199"             }
	, {      "ISO-8859-14", "ISO_8859-14"            }
	, {      "ISO-8859-14", "ISO_8859-14:1998"       }
	, {      "ISO-8859-14", "L8"                     }
	, {      "ISO-8859-14", "LATIN8"                 }
	, {      "ISO-8859-15", "ISO8859-15"             }
	, {      "ISO-8859-15", "ISO-8859-15"            }
	, {      "ISO-8859-15", "ISO-IR-203"             }
	, {      "ISO-8859-15", "ISO_8859-15"            }
	, {      "ISO-8859-15", "ISO_8859-15:1998"       }
	, {      "ISO-8859-15", "iso815"                 }
	, {      "ISO-8859-15", "iso885915"              }
	, {      "ISO-8859-16", "ISO-8859-16"            }
	, {      "ISO-8859-16", "ISO-IR-226"             }
	, {      "ISO-8859-16", "ISO_8859-16"            }
	, {      "ISO-8859-16", "ISO_8859-16:2000"       }
	, {       "ISO-8859-2", "ISO-8859-2"             }
	, {       "ISO-8859-2", "CSISOLATIN2"            }
	, {       "ISO-8859-2", "ISO8859-2"              }
	, {       "ISO-8859-2", "ISO-IR-101"             }
	, {       "ISO-8859-2", "ISO_8859-2"             }
	, {       "ISO-8859-2", "ISO_8859-2:1987"        }
	, {       "ISO-8859-2", "L2"                     }
	, {       "ISO-8859-2", "LATIN2"                 }
	, {       "ISO-8859-2", "iso82"                  }
	, {       "ISO-8859-2", "iso88592"               }
	, {       "ISO-8859-3", "ISO-8859-3"             }
	, {       "ISO-8859-3", "CSISOLATIN3"            }
	, {       "ISO-8859-3", "ISO-IR-109"             }
	, {       "ISO-8859-3", "ISO_8859-3"             }
	, {       "ISO-8859-3", "ISO_8859-3:1988"        }
	, {       "ISO-8859-3", "L3"                     }
	, {       "ISO-8859-3", "LATIN3"                 }
	, {       "ISO-8859-3", "iso83"                  }
	, {       "ISO-8859-3", "iso88593"               }
	, {       "ISO-8859-4", "ISO8859-4"              }
	, {       "ISO-8859-4", "CSISOLATIN4"            }
	, {       "ISO-8859-4", "ISO-8859-4"             }
	, {       "ISO-8859-4", "ISO-IR-110"             }
	, {       "ISO-8859-4", "ISO_8859-4"             }
	, {       "ISO-8859-4", "ISO_8859-4:1988"        }
	, {       "ISO-8859-4", "L4"                     }
	, {       "ISO-8859-4", "LATIN4"                 }
	, {       "ISO-8859-4", "iso84"                  }
	, {       "ISO-8859-4", "iso88594"               }
	, {       "ISO-8859-5", "ISO-8859-5"             }
	, {       "ISO-8859-5", "CSISOLATINCYRILLIC"     }
	, {       "ISO-8859-5", "CYRILLIC"               }
	, {       "ISO-8859-5", "ISO8859-5"              }
	, {       "ISO-8859-5", "ISO-IR-144"             }
	, {       "ISO-8859-5", "ISO8859-5"              }
	, {       "ISO-8859-5", "ISO_8859-5"             }
	, {       "ISO-8859-5", "ISO_8859-5:1988"        }
	, {       "ISO-8859-5", "iso85"                  }
	, {       "ISO-8859-5", "iso88595"               }
	, {       "ISO-8859-6", "ISO-8859-6"             }
	, {       "ISO-8859-6", "ARABIC"                 }
	, {       "ISO-8859-6", "ASMO-708"               }
	, {       "ISO-8859-6", "CSISOLATINARABIC"       }
	, {       "ISO-8859-6", "ECMA-114"               }
	, {       "ISO-8859-6", "ISO-IR-127"             }
	, {       "ISO-8859-6", "ISO8859-6"              }
	, {       "ISO-8859-6", "ISO_8859-6"             }
	, {       "ISO-8859-6", "ISO_8859-6:1987"        }
	, {       "ISO-8859-6", "iso86"                  }
	, {       "ISO-8859-6", "iso88596"               }
	, {       "ISO-8859-7", "ISO-8859-7"             }
	, {       "ISO-8859-7", "CSISOLATINGREEK"        }
	, {       "ISO-8859-7", "ECMA-118"               }
	, {       "ISO-8859-7", "ELOT_928"               }
	, {       "ISO-8859-7", "GREEK"                  }
	, {       "ISO-8859-7", "GREEK8"                 }
	, {       "ISO-8859-7", "ISO-IR-126"             }
	, {       "ISO-8859-7", "ISO8859-7"              }
	, {       "ISO-8859-7", "ISO_8859-7"             }
	, {       "ISO-8859-7", "ISO_8859-7:1987"        }
	, {       "ISO-8859-7", "iso87"                  }
	, {       "ISO-8859-7", "iso88597"               }
	, {       "ISO-8859-8", "ISO-8859-8"             }
	, {       "ISO-8859-8", "CSISOLATINHEBREW"       }
	, {       "ISO-8859-8", "HEBREW"                 }
	, {       "ISO-8859-8", "ISO8859-8"              }
	, {       "ISO-8859-8", "ISO-IR-138"             }
	, {       "ISO-8859-8", "ISO_8859-8"             }
	, {       "ISO-8859-8", "ISO_8859-8:1988"        }
	, {       "ISO-8859-8", "iso88"                  }
	, {       "ISO-8859-8", "iso88598"               }
	, {       "ISO-8859-9", "ISO-8859-9"             }
	, {       "ISO-8859-9", "CSISOLATIN5"            }
	, {       "ISO-8859-9", "ISO-IR-148"             }
	, {       "ISO-8859-9", "ISO8859-9"              }
	, {       "ISO-8859-9", "ISO_8859-9"             }
	, {       "ISO-8859-9", "ISO_8859-9:1989"        }
	, {       "ISO-8859-9", "L5"                     }
	, {       "ISO-8859-9", "LATIN5"                 }
	, {       "ISO-8859-9", "iso88599"               }
	, {       "ISO-8859-9", "iso89"                  }
	, {        "ISO-IR-14", "ISO-IR-14"              }
	, {        "ISO-IR-14", "CSISO14JISC6220RO"      }
	, {        "ISO-IR-14", "ISO646-JP"              }
	, {        "ISO-IR-14", "JIS_C6220-1969-RO"      }
	, {        "ISO-IR-14", "JP"                     }
	, {       "ISO-IR-149", "ISO-IR-149"             }
	, {       "ISO-IR-149", "CSKSC56011987"          }
	, {       "ISO-IR-149", "KOREAN"                 }
	, {       "ISO-IR-149", "KSC_5601"               }
	, {       "ISO-IR-149", "KS_C_5601-1987"         }
	, {       "ISO-IR-149", "KS_C_5601-1989"         }
	, {       "ISO-IR-159", "ISO-IR-159"             }
	, {       "ISO-IR-159", "CSISO159JISX02121990"   }
	, {       "ISO-IR-159", "JIS_X0212"              }
	, {       "ISO-IR-159", "JIS_X0212-1990"         }
	, {       "ISO-IR-159", "JIS_X0212.1990-0"       }
	, {       "ISO-IR-159", "X0212"                  }
	, {       "ISO-IR-166", "ISO-IR-166"             }
	, {       "ISO-IR-166", "TIS-620"                }
	, {       "ISO-IR-166", "TIS620"                 }
	, {       "ISO-IR-166", "TIS620-0"               }
	, {       "ISO-IR-166", "TIS620.2529-1"          }
	, {       "ISO-IR-166", "TIS620.2533-0"          }
	, {       "ISO-IR-166", "TIS620.2533-1"          }
	, {       "ISO-IR-166", "thai8"                  }
	, {       "ISO-IR-166", "tis620"                 }
	, {        "ISO-IR-87", "ISO-IR-87"              }
	, {        "ISO-IR-87", "CSISO87JISX0208"        }
	, {        "ISO-IR-87", "JIS0208"                }
	, {        "ISO-IR-87", "JIS_C6226-1983"         }
	, {        "ISO-IR-87", "JIS_X0208"              }
	, {        "ISO-IR-87", "JIS_X0208-1983"         }
	, {        "ISO-IR-87", "JIS_X0208-1990"         }
	, {        "ISO-IR-87", "X0208"                  }
	, {             "JAVA", "JAVA"                   }
	, {    "JISX0201-1976", "JISX0201-1976"          }
	, {    "JISX0201-1976", "CSHALFWIDTHKATAKANA"    }
	, {    "JISX0201-1976", "JIS_X0201"              }
	, {    "JISX0201-1976", "X0201"                  }
	, {           "KOI8-R", "KOI8-R"                 }
	, {           "KOI8-R", "CSKOI8R"                }
	, {          "KOI8-RU", "KOI8-RU"                }
	, {           "KOI8-T", "KOI8-T"                 }
	, {           "KOI8-U", "KOI8-U"                 }
	, {              "MAC", "MAC"                    }
	, {              "MAC", "CSMACINTOSH"            }
	, {              "MAC", "MACINTOSH"              }
	, {              "MAC", "MACROMAN"               }
	, {        "MACARABIC", "MACARABIC"              }
	, { "MACCENTRALEUROPE", "MACCENTRALEUROPE"       }
	, {      "MACCROATIAN", "MACCROATIAN"            }
	, {      "MACCYRILLIC", "MACCYRILLIC"            }
	, {         "MACGREEK", "MACGREEK"               }
	, {        "MACHEBREW", "MACHEBREW"              }
	, {       "MACICELAND", "MACICELAND"             }
	, {       "MACROMANIA", "MACROMANIA"             }
	, {          "MACTHAI", "MACTHAI"                }
	, {       "MACTURKISH", "MACTURKISH"             }
	, {       "MACUKRAINE", "MACUKRAINE"             }
	, {        "MULELAO-1", "MULELAO-1"              }
	, {         "NEXTSTEP", "NEXTSTEP"               }
	, {           "ROMAN8", "ROMAN8"                 }
	, {           "ROMAN8", "CSHPROMAN8"             }
	, {           "ROMAN8", "HP-ROMAN8"              }
	, {           "ROMAN8", "R8"                     }
	, {           "ROMAN8", "roma8"                  }
	, {           "ROMAN8", "roman8"                 }
	, {             "SJIS", "SJIS"                   }
	, {             "SJIS", "CSSHIFTJIS"             }
	, {             "SJIS", "MS_KANJI"               }
	, {             "SJIS", "SHIFT-JIS"              }
	, {             "SJIS", "SHIFT_JIS"              }
	, {             "SJIS", "sjis"                   }
	, {             "TCVN", "TCVN"                   }
	, {             "TCVN", "TCVN-5712"              }
	, {             "TCVN", "TCVN5712-1"             }
	, {             "TCVN", "TCVN5712-1:1993"        }
	, {            "UCS-4", "UCS-4"                  }
	, {            "UCS-4", "CSUCS4"                 }
	, {            "UCS-4", "ISO-10646-UCS-4"        }
	, {            "UCS-4", "UCS4"                   }
	, {            "UCS-4", "ucs4"                   }
	, {   "UCS-4-INTERNAL", "UCS-4-INTERNAL"         }
	, {    "UCS-4-SWAPPED", "UCS-4-SWAPPED"          }
	, {          "UCS-4BE", "UCS-4BE"                }
	, {          "UCS-4LE", "UCS-4LE"                }
	, {           "UTF-16", "UTF-16"                 }
	, {           "UTF-16", "UTF16"                  }
	, {         "UTF-16BE", "UTF-16BE"               }
	, {         "UTF-16LE", "UTF-16LE"               }
	, {           "UTF-32", "UTF-32"                 }
	, {         "UTF-32BE", "UTF-32BE"               }
	, {         "UTF-32LE", "UTF-32LE"               }
	, {            "UTF-7", "UTF-7"                  }
	, {            "UTF-7", "CSUNICODE11UTF7"        }
	, {            "UTF-7", "UNICODE-1-1-UTF-7"      }
	, {            "UTF-7", "UTF7"                   }
	, {           "VISCII", "VISCII"                 }
	, {           "VISCII", "CSVISCII"               }
	, {           "VISCII", "VISCII1.1-1"            }

	/* 
	 * The following are noted in Tru64 manuals, but 
	 * have no canonical names in FreeTDS
	 *
	 * 	TACTIS          TACTIS codeset	               
	 * 	dechanyu        DEC Hanyu codeset              
	 * 	dechanzi        DEC Hanzi codeset              
	 * 	deckanji        DEC Kanji codeset              
	 * 	deckorean       DEC Korean codeset             
	 * 	sdeckanji       Super DEC Kanji codeset        
	 */
	 
	/* no stopper row; add your own */
#endif
