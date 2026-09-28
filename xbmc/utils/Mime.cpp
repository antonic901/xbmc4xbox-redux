/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Mime.h"

#include "FileItem.h"
#include "URIUtils.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/StringUtils.h"
#include "video/VideoInfoTag.h"

#include <algorithm>

namespace
{
std::map<std::string, std::string> CreateMimeTypes()
{
  std::map<std::string, std::string> types;
#if 0
  // TODO: check how much memory this map will take
  types.insert(std::make_pair("3dm", "x-world/x-3dmf"));
  types.insert(std::make_pair("3dmf", "x-world/x-3dmf"));
  types.insert(std::make_pair("3fr", "image/3fr"));
  types.insert(std::make_pair("a", "application/octet-stream"));
  types.insert(std::make_pair("aab", "application/x-authorware-bin"));
  types.insert(std::make_pair("aam", "application/x-authorware-map"));
  types.insert(std::make_pair("aas", "application/x-authorware-seg"));
  types.insert(std::make_pair("abc", "text/vnd.abc"));
  types.insert(std::make_pair("acgi", "text/html"));
  types.insert(std::make_pair("afl", "video/animaflex"));
  types.insert(std::make_pair("ai", "application/postscript"));
  types.insert(std::make_pair("aif", "audio/aiff"));
  types.insert(std::make_pair("aifc", "audio/x-aiff"));
  types.insert(std::make_pair("aiff", "audio/aiff"));
  types.insert(std::make_pair("aim", "application/x-aim"));
  types.insert(std::make_pair("aip", "text/x-audiosoft-intra"));
  types.insert(std::make_pair("ani", "application/x-navi-animation"));
  types.insert(std::make_pair("aos", "application/x-nokia-9000-communicator-add-on-software"));
  types.insert(std::make_pair("apng", "image/apng"));
  types.insert(std::make_pair("aps", "application/mime"));
  types.insert(std::make_pair("arc", "application/octet-stream"));
  types.insert(std::make_pair("arj", "application/arj"));
  types.insert(std::make_pair("art", "image/x-jg"));
  types.insert(std::make_pair("arw", "image/arw"));
  types.insert(std::make_pair("asf", "video/x-ms-asf"));
  types.insert(std::make_pair("asm", "text/x-asm"));
  types.insert(std::make_pair("asp", "text/asp"));
  types.insert(std::make_pair("asx", "video/x-ms-asf"));
  types.insert(std::make_pair("au", "audio/basic"));
  types.insert(std::make_pair("avi", "video/avi"));
  types.insert(std::make_pair("avs", "video/avs-video"));
  types.insert(std::make_pair("bcpio", "application/x-bcpio"));
  types.insert(std::make_pair("bin", "application/octet-stream"));
  types.insert(std::make_pair("bm", "image/bmp"));
  types.insert(std::make_pair("bmp", "image/bmp"));
  types.insert(std::make_pair("boo", "application/book"));
  types.insert(std::make_pair("book", "application/book"));
  types.insert(std::make_pair("boz", "application/x-bzip2"));
  types.insert(std::make_pair("bsh", "application/x-bsh"));
  types.insert(std::make_pair("bz", "application/x-bzip"));
  types.insert(std::make_pair("bz2", "application/x-bzip2"));
  types.insert(std::make_pair("c", "text/plain"));
  types.insert(std::make_pair("c++", "text/plain"));
  types.insert(std::make_pair("cat", "application/vnd.ms-pki.seccat"));
  types.insert(std::make_pair("cc", "text/plain"));
  types.insert(std::make_pair("ccad", "application/clariscad"));
  types.insert(std::make_pair("cco", "application/x-cocoa"));
  types.insert(std::make_pair("cdf", "application/cdf"));
  types.insert(std::make_pair("cer", "application/pkix-cert"));
  types.insert(std::make_pair("cer", "application/x-x509-ca-cert"));
  types.insert(std::make_pair("cha", "application/x-chat"));
  types.insert(std::make_pair("chat", "application/x-chat"));
  types.insert(std::make_pair("class", "application/java"));
  types.insert(std::make_pair("com", "application/octet-stream"));
  types.insert(std::make_pair("conf", "text/plain"));
  types.insert(std::make_pair("cpio", "application/x-cpio"));
  types.insert(std::make_pair("cpp", "text/x-c"));
  types.insert(std::make_pair("cpt", "application/x-cpt"));
  types.insert(std::make_pair("crl", "application/pkcs-crl"));
  types.insert(std::make_pair("crt", "application/pkix-cert"));
  types.insert(std::make_pair("cr2", "image/cr2"));
  types.insert(std::make_pair("crw", "image/crw"));
  types.insert(std::make_pair("csh", "application/x-csh"));
  types.insert(std::make_pair("css", "text/css"));
  types.insert(std::make_pair("cxx", "text/plain"));
  types.insert(std::make_pair("dcr", "application/x-director"));
  types.insert(std::make_pair("deepv", "application/x-deepv"));
  types.insert(std::make_pair("def", "text/plain"));
  types.insert(std::make_pair("der", "application/x-x509-ca-cert"));
  types.insert(std::make_pair("dif", "video/x-dv"));
  types.insert(std::make_pair("dir", "application/x-director"));
  types.insert(std::make_pair("dl", "video/dl"));
  types.insert(std::make_pair("divx", "video/x-msvideo"));
  types.insert(std::make_pair("dng", "image/dng"));
  types.insert(std::make_pair("doc", "application/msword"));
  types.insert(std::make_pair("docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"));
  types.insert(std::make_pair("dot", "application/msword"));
  types.insert(std::make_pair("dp", "application/commonground"));
  types.insert(std::make_pair("drw", "application/drafting"));
  types.insert(std::make_pair("dump", "application/octet-stream"));
  types.insert(std::make_pair("dv", "video/x-dv"));
  types.insert(std::make_pair("dvi", "application/x-dvi"));
  types.insert(std::make_pair("dwf", "model/vnd.dwf"));
  types.insert(std::make_pair("dwg", "image/vnd.dwg"));
  types.insert(std::make_pair("dxf", "image/vnd.dwg"));
  types.insert(std::make_pair("dxr", "application/x-director"));
  types.insert(std::make_pair("el", "text/x-script.elisp"));
  types.insert(std::make_pair("elc", "application/x-elc"));
  types.insert(std::make_pair("env", "application/x-envoy"));
  types.insert(std::make_pair("eps", "application/postscript"));
  types.insert(std::make_pair("erf", "image/erf"));
  types.insert(std::make_pair("es", "application/x-esrehber"));
  types.insert(std::make_pair("etx", "text/x-setext"));
  types.insert(std::make_pair("evy", "application/envoy"));
  types.insert(std::make_pair("exe", "application/octet-stream"));
  types.insert(std::make_pair("f", "text/x-fortran"));
  types.insert(std::make_pair("f77", "text/x-fortran"));
  types.insert(std::make_pair("f90", "text/x-fortran"));
  types.insert(std::make_pair("fdf", "application/vnd.fdf"));
  types.insert(std::make_pair("fif", "image/fif"));
  types.insert(std::make_pair("flac", "audio/flac"));
  types.insert(std::make_pair("fli", "video/fli"));
  types.insert(std::make_pair("flo", "image/florian"));
  types.insert(std::make_pair("flv", "video/x-flv"));
  types.insert(std::make_pair("flx", "text/vnd.fmi.flexstor"));
  types.insert(std::make_pair("fmf", "video/x-atomic3d-feature"));
  types.insert(std::make_pair("for", "text/plain"));
  types.insert(std::make_pair("for", "text/x-fortran"));
  types.insert(std::make_pair("fpx", "image/vnd.fpx"));
  types.insert(std::make_pair("frl", "application/freeloader"));
  types.insert(std::make_pair("funk", "audio/make"));
  types.insert(std::make_pair("g", "text/plain"));
  types.insert(std::make_pair("g3", "image/g3fax"));
  types.insert(std::make_pair("gif", "image/gif"));
  types.insert(std::make_pair("gl", "video/x-gl"));
  types.insert(std::make_pair("gsd", "audio/x-gsm"));
  types.insert(std::make_pair("gsm", "audio/x-gsm"));
  types.insert(std::make_pair("gsp", "application/x-gsp"));
  types.insert(std::make_pair("gss", "application/x-gss"));
  types.insert(std::make_pair("gtar", "application/x-gtar"));
  types.insert(std::make_pair("gz", "application/x-compressed"));
  types.insert(std::make_pair("gzip", "application/x-gzip"));
  types.insert(std::make_pair("h", "text/plain"));
  types.insert(std::make_pair("hdf", "application/x-hdf"));
  types.insert(std::make_pair("heic", "image/heic"));
  types.insert(std::make_pair("heif", "image/heif"));
  types.insert(std::make_pair("help", "application/x-helpfile"));
  types.insert(std::make_pair("hgl", "application/vnd.hp-hpgl"));
  types.insert(std::make_pair("hh", "text/plain"));
  types.insert(std::make_pair("hlb", "text/x-script"));
  types.insert(std::make_pair("hlp", "application/hlp"));
  types.insert(std::make_pair("hpg", "application/vnd.hp-hpgl"));
  types.insert(std::make_pair("hpgl", "application/vnd.hp-hpgl"));
  types.insert(std::make_pair("hqx", "application/binhex"));
  types.insert(std::make_pair("hta", "application/hta"));
  types.insert(std::make_pair("htc", "text/x-component"));
  types.insert(std::make_pair("htm", "text/html"));
  types.insert(std::make_pair("html", "text/html"));
  types.insert(std::make_pair("htmls", "text/html"));
  types.insert(std::make_pair("htt", "text/webviewhtml"));
  types.insert(std::make_pair("htx", "text/html"));
  types.insert(std::make_pair("ice", "x-conference/x-cooltalk"));
  types.insert(std::make_pair("ico", "image/x-icon"));
  types.insert(std::make_pair("idc", "text/plain"));
  types.insert(std::make_pair("ief", "image/ief"));
  types.insert(std::make_pair("iefs", "image/ief"));
  types.insert(std::make_pair("iges", "application/iges"));
  types.insert(std::make_pair("igs", "application/iges"));
  types.insert(std::make_pair("ima", "application/x-ima"));
  types.insert(std::make_pair("imap", "application/x-httpd-imap"));
  types.insert(std::make_pair("inf", "application/inf"));
  types.insert(std::make_pair("ins", "application/x-internet-signup"));
  types.insert(std::make_pair("ip", "application/x-ip2"));
  types.insert(std::make_pair("isu", "video/x-isvideo"));
  types.insert(std::make_pair("it", "audio/it"));
  types.insert(std::make_pair("iv", "application/x-inventor"));
  types.insert(std::make_pair("ivr", "i-world/i-vrml"));
  types.insert(std::make_pair("ivy", "application/x-livescreen"));
  types.insert(std::make_pair("jam", "audio/x-jam"));
  types.insert(std::make_pair("jav", "text/x-java-source"));
  types.insert(std::make_pair("java", "text/x-java-source"));
  types.insert(std::make_pair("jcm", "application/x-java-commerce"));
  types.insert(std::make_pair("jfif", "image/jpeg"));
  types.insert(std::make_pair("jp2", "image/jp2"));
  types.insert(std::make_pair("jfif-tbnl", "image/jpeg"));
  types.insert(std::make_pair("jpe", "image/jpeg"));
  types.insert(std::make_pair("jpeg", "image/jpeg"));
  types.insert(std::make_pair("jpg", "image/jpeg"));
  types.insert(std::make_pair("jps", "image/x-jps"));
  types.insert(std::make_pair("js", "application/javascript"));
  types.insert(std::make_pair("json", "application/json"));
  types.insert(std::make_pair("jut", "image/jutvision"));
  types.insert(std::make_pair("kar", "music/x-karaoke"));
  types.insert(std::make_pair("kdc", "image/kdc"));
  types.insert(std::make_pair("ksh", "text/x-script.ksh"));
  types.insert(std::make_pair("la", "audio/nspaudio"));
  types.insert(std::make_pair("lam", "audio/x-liveaudio"));
  types.insert(std::make_pair("latex", "application/x-latex"));
  types.insert(std::make_pair("lha", "application/lha"));
  types.insert(std::make_pair("lhx", "application/octet-stream"));
  types.insert(std::make_pair("list", "text/plain"));
  types.insert(std::make_pair("lma", "audio/nspaudio"));
  types.insert(std::make_pair("log", "text/plain"));
  types.insert(std::make_pair("lsp", "application/x-lisp"));
  types.insert(std::make_pair("lst", "text/plain"));
  types.insert(std::make_pair("lsx", "text/x-la-asf"));
  types.insert(std::make_pair("ltx", "application/x-latex"));
  types.insert(std::make_pair("lzh", "application/x-lzh"));
  types.insert(std::make_pair("lzx", "application/lzx"));
  types.insert(std::make_pair("m", "text/x-m"));
  types.insert(std::make_pair("m1v", "video/mpeg"));
  types.insert(std::make_pair("m2a", "audio/mpeg"));
  types.insert(std::make_pair("m2v", "video/mpeg"));
  types.insert(std::make_pair("m3u", "audio/x-mpegurl"));
  types.insert(std::make_pair("man", "application/x-troff-man"));
  types.insert(std::make_pair("map", "application/x-navimap"));
  types.insert(std::make_pair("mar", "text/plain"));
  types.insert(std::make_pair("mbd", "application/mbedlet"));
  types.insert(std::make_pair("mc$", "application/x-magic-cap-package-1.0"));
  types.insert(std::make_pair("mcd", "application/x-mathcad"));
  types.insert(std::make_pair("mcf", "text/mcf"));
  types.insert(std::make_pair("mcp", "application/netmc"));
  types.insert(std::make_pair("mdc", "image/mdc"));
  types.insert(std::make_pair("me", "application/x-troff-me"));
  types.insert(std::make_pair("mef", "image/mef"));
  types.insert(std::make_pair("mht", "message/rfc822"));
  types.insert(std::make_pair("mhtml", "message/rfc822"));
  types.insert(std::make_pair("mid", "audio/midi"));
  types.insert(std::make_pair("midi", "audio/midi"));
  types.insert(std::make_pair("mif", "application/x-mif"));
  types.insert(std::make_pair("mime", "message/rfc822"));
  types.insert(std::make_pair("mjf", "audio/x-vnd.audioexplosion.mjuicemediafile"));
  types.insert(std::make_pair("mjpg", "video/x-motion-jpeg"));
  types.insert(std::make_pair("mka", "audio/x-matroska"));
  types.insert(std::make_pair("mkv", "video/x-matroska"));
  types.insert(std::make_pair("mk3d", "video/x-matroska-3d"));
  types.insert(std::make_pair("mm", "application/x-meme"));
  types.insert(std::make_pair("mme", "application/base64"));
  types.insert(std::make_pair("mod", "audio/mod"));
  types.insert(std::make_pair("moov", "video/quicktime"));
  types.insert(std::make_pair("mov", "video/quicktime"));
  types.insert(std::make_pair("movie", "video/x-sgi-movie"));
  types.insert(std::make_pair("mos", "image/mos"));
  types.insert(std::make_pair("mp2", "audio/mpeg"));
  types.insert(std::make_pair("mp3", "audio/mpeg3"));
  types.insert(std::make_pair("mp4", "video/mp4"));
  types.insert(std::make_pair("mpa", "audio/mpeg"));
  types.insert(std::make_pair("mpc", "application/x-project"));
  types.insert(std::make_pair("mpe", "video/mpeg"));
  types.insert(std::make_pair("mpeg", "video/mpeg"));
  types.insert(std::make_pair("mpg", "video/mpeg"));
  types.insert(std::make_pair("mpga", "audio/mpeg"));
  types.insert(std::make_pair("mpp", "application/vnd.ms-project"));
  types.insert(std::make_pair("mpt", "application/x-project"));
  types.insert(std::make_pair("mpv", "application/x-project"));
  types.insert(std::make_pair("mpx", "application/x-project"));
  types.insert(std::make_pair("mrc", "application/marc"));
  types.insert(std::make_pair("mrw", "image/mrw"));
  types.insert(std::make_pair("ms", "application/x-troff-ms"));
  types.insert(std::make_pair("mv", "video/x-sgi-movie"));
  types.insert(std::make_pair("my", "audio/make"));
  types.insert(std::make_pair("mzz", "application/x-vnd.audioexplosion.mzz"));
  types.insert(std::make_pair("nap", "image/naplps"));
  types.insert(std::make_pair("naplps", "image/naplps"));
  types.insert(std::make_pair("nc", "application/x-netcdf"));
  types.insert(std::make_pair("ncm", "application/vnd.nokia.configuration-message"));
  types.insert(std::make_pair("nef", "image/nef"));
  types.insert(std::make_pair("nfo", "text/xml"));
  types.insert(std::make_pair("nif", "image/x-niff"));
  types.insert(std::make_pair("niff", "image/x-niff"));
  types.insert(std::make_pair("nix", "application/x-mix-transfer"));
  types.insert(std::make_pair("nrw", "image/nrw"));
  types.insert(std::make_pair("nsc", "application/x-conference"));
  types.insert(std::make_pair("nvd", "application/x-navidoc"));
  types.insert(std::make_pair("o", "application/octet-stream"));
  types.insert(std::make_pair("oda", "application/oda"));
  types.insert(std::make_pair("ogg", "audio/ogg"));
  types.insert(std::make_pair("omc", "application/x-omc"));
  types.insert(std::make_pair("omcd", "application/x-omcdatamaker"));
  types.insert(std::make_pair("omcr", "application/x-omcregerator"));
  types.insert(std::make_pair("orf", "image/orf"));
  types.insert(std::make_pair("p", "text/x-pascal"));
  types.insert(std::make_pair("p10", "application/pkcs10"));
  types.insert(std::make_pair("p12", "application/pkcs-12"));
  types.insert(std::make_pair("p7a", "application/x-pkcs7-signature"));
  types.insert(std::make_pair("p7c", "application/pkcs7-mime"));
  types.insert(std::make_pair("p7m", "application/pkcs7-mime"));
  types.insert(std::make_pair("p7r", "application/x-pkcs7-certreqresp"));
  types.insert(std::make_pair("p7s", "application/pkcs7-signature"));
  types.insert(std::make_pair("part", "application/pro_eng"));
  types.insert(std::make_pair("pas", "text/pascal"));
  types.insert(std::make_pair("pbm", "image/x-portable-bitmap"));
  types.insert(std::make_pair("pcl", "application/vnd.hp-pcl"));
  types.insert(std::make_pair("pct", "image/x-pict"));
  types.insert(std::make_pair("pcx", "image/x-pcx"));
  types.insert(std::make_pair("pdb", "chemical/x-pdb"));
  types.insert(std::make_pair("pdf", "application/pdf"));
  types.insert(std::make_pair("pef", "image/pef"));
  types.insert(std::make_pair("pfunk", "audio/make.my.funk"));
  types.insert(std::make_pair("pgm", "image/x-portable-greymap"));
  types.insert(std::make_pair("pic", "image/pict"));
  types.insert(std::make_pair("pict", "image/pict"));
  types.insert(std::make_pair("pkg", "application/x-newton-compatible-pkg"));
  types.insert(std::make_pair("pko", "application/vnd.ms-pki.pko"));
  types.insert(std::make_pair("pl", "text/x-script.perl"));
  types.insert(std::make_pair("plx", "application/x-pixclscript"));
  types.insert(std::make_pair("pm", "text/x-script.perl-module"));
  types.insert(std::make_pair("pm4", "application/x-pagemaker"));
  types.insert(std::make_pair("pm5", "application/x-pagemaker"));
  types.insert(std::make_pair("png", "image/png"));
  types.insert(std::make_pair("pnm", "application/x-portable-anymap"));
  types.insert(std::make_pair("pot", "application/vnd.ms-powerpoint"));
  types.insert(std::make_pair("pov", "model/x-pov"));
  types.insert(std::make_pair("ppa", "application/vnd.ms-powerpoint"));
  types.insert(std::make_pair("ppm", "image/x-portable-pixmap"));
  types.insert(std::make_pair("pps", "application/mspowerpoint"));
  types.insert(std::make_pair("ppt", "application/mspowerpoint"));
  types.insert(std::make_pair("ppz", "application/mspowerpoint"));
  types.insert(std::make_pair("pre", "application/x-freelance"));
  types.insert(std::make_pair("prt", "application/pro_eng"));
  types.insert(std::make_pair("ps", "application/postscript"));
  types.insert(std::make_pair("psd", "application/octet-stream"));
  types.insert(std::make_pair("pvu", "paleovu/x-pv"));
  types.insert(std::make_pair("pwz", "application/vnd.ms-powerpoint"));
  types.insert(std::make_pair("py", "text/x-script.python"));
  types.insert(std::make_pair("pyc", "application/x-bytecode.python"));
  types.insert(std::make_pair("qcp", "audio/vnd.qcelp"));
  types.insert(std::make_pair("qd3", "x-world/x-3dmf"));
  types.insert(std::make_pair("qd3d", "x-world/x-3dmf"));
  types.insert(std::make_pair("qif", "image/x-quicktime"));
  types.insert(std::make_pair("qt", "video/quicktime"));
  types.insert(std::make_pair("qtc", "video/x-qtc"));
  types.insert(std::make_pair("qti", "image/x-quicktime"));
  types.insert(std::make_pair("qtif", "image/x-quicktime"));
  types.insert(std::make_pair("ra", "audio/x-realaudio"));
  types.insert(std::make_pair("raf", "image/raf"));
  types.insert(std::make_pair("ram", "audio/x-pn-realaudio"));
  types.insert(std::make_pair("ras", "image/cmu-raster"));
  types.insert(std::make_pair("rast", "image/cmu-raster"));
  types.insert(std::make_pair("raw", "image/raw"));
  types.insert(std::make_pair("rexx", "text/x-script.rexx"));
  types.insert(std::make_pair("rf", "image/vnd.rn-realflash"));
  types.insert(std::make_pair("rgb", "image/x-rgb"));
  types.insert(std::make_pair("rm", "application/vnd.rn-realmedia"));
  types.insert(std::make_pair("rmi", "audio/mid"));
  types.insert(std::make_pair("rmm", "audio/x-pn-realaudio"));
  types.insert(std::make_pair("rmp", "audio/x-pn-realaudio"));
  types.insert(std::make_pair("rng", "application/ringing-tones"));
  types.insert(std::make_pair("rnx", "application/vnd.rn-realplayer"));
  types.insert(std::make_pair("roff", "application/x-troff"));
  types.insert(std::make_pair("rp", "image/vnd.rn-realpix"));
  types.insert(std::make_pair("rpm", "audio/x-pn-realaudio-plugin"));
  types.insert(std::make_pair("rt", "text/richtext"));
  types.insert(std::make_pair("rtf", "text/richtext"));
  types.insert(std::make_pair("rtx", "text/richtext"));
  types.insert(std::make_pair("rv", "video/vnd.rn-realvideo"));
  types.insert(std::make_pair("rw2", "image/rw2"));
  types.insert(std::make_pair("s", "text/x-asm"));
  types.insert(std::make_pair("s3m", "audio/s3m"));
  types.insert(std::make_pair("saveme", "application/octet-stream"));
  types.insert(std::make_pair("sbk", "application/x-tbook"));
  types.insert(std::make_pair("scm", "video/x-scm"));
  types.insert(std::make_pair("sdml", "text/plain"));
  types.insert(std::make_pair("sdp", "application/sdp"));
  types.insert(std::make_pair("sdr", "application/sounder"));
  types.insert(std::make_pair("sea", "application/sea"));
  types.insert(std::make_pair("set", "application/set"));
  types.insert(std::make_pair("sgm", "text/sgml"));
  types.insert(std::make_pair("sgml", "text/sgml"));
  types.insert(std::make_pair("sh", "text/x-script.sh"));
  types.insert(std::make_pair("shar", "application/x-bsh"));
  types.insert(std::make_pair("shtml", "text/x-server-parsed-html"));
  types.insert(std::make_pair("sid", "audio/x-psid"));
  types.insert(std::make_pair("sit", "application/x-stuffit"));
  types.insert(std::make_pair("skd", "application/x-koan"));
  types.insert(std::make_pair("skm", "application/x-koan"));
  types.insert(std::make_pair("skp", "application/x-koan"));
  types.insert(std::make_pair("skt", "application/x-koan"));
  types.insert(std::make_pair("sl", "application/x-seelogo"));
  types.insert(std::make_pair("smi", "application/smil"));
  types.insert(std::make_pair("smil", "application/smil"));
  types.insert(std::make_pair("snd", "audio/basic"));
  types.insert(std::make_pair("sol", "application/solids"));
  types.insert(std::make_pair("spc", "text/x-speech"));
  types.insert(std::make_pair("spl", "application/futuresplash"));
  types.insert(std::make_pair("spr", "application/x-sprite"));
  types.insert(std::make_pair("sprite", "application/x-sprite"));
  types.insert(std::make_pair("src", "application/x-wais-source"));
  types.insert(std::make_pair("srw", "image/srw"));
  types.insert(std::make_pair("ssi", "text/x-server-parsed-html"));
  types.insert(std::make_pair("ssm", "application/streamingmedia"));
  types.insert(std::make_pair("sst", "application/vnd.ms-pki.certstore"));
  types.insert(std::make_pair("step", "application/step"));
  types.insert(std::make_pair("stl", "application/sla"));
  types.insert(std::make_pair("stp", "application/step"));
  types.insert(std::make_pair("sup", "application/x-pgs"));
  types.insert(std::make_pair("sv4cpio", "application/x-sv4cpio"));
  types.insert(std::make_pair("sv4crc", "application/x-sv4crc"));
  types.insert(std::make_pair("svf", "image/vnd.dwg"));
  types.insert(std::make_pair("svg", "image/svg+xml"));
  types.insert(std::make_pair("svr", "application/x-world"));
  types.insert(std::make_pair("swf", "application/x-shockwave-flash"));
  types.insert(std::make_pair("t", "application/x-troff"));
  types.insert(std::make_pair("talk", "text/x-speech"));
  types.insert(std::make_pair("tar", "application/x-tar"));
  types.insert(std::make_pair("tbk", "application/toolbook"));
  types.insert(std::make_pair("tcl", "text/x-script.tcl"));
  types.insert(std::make_pair("tcsh", "text/x-script.tcsh"));
  types.insert(std::make_pair("tex", "application/x-tex"));
  types.insert(std::make_pair("texi", "application/x-texinfo"));
  types.insert(std::make_pair("texinfo", "application/x-texinfo"));
  types.insert(std::make_pair("text", "text/plain"));
  types.insert(std::make_pair("tgz", "application/x-compressed"));
  types.insert(std::make_pair("tif", "image/tiff"));
  types.insert(std::make_pair("tiff", "image/tiff"));
  types.insert(std::make_pair("tr", "application/x-troff"));
  types.insert(std::make_pair("ts", "video/mp2t"));
  types.insert(std::make_pair("tsi", "audio/tsp-audio"));
  types.insert(std::make_pair("tsp", "audio/tsplayer"));
  types.insert(std::make_pair("tsv", "text/tab-separated-values"));
  types.insert(std::make_pair("turbot", "image/florian"));
  types.insert(std::make_pair("txt", "text/plain"));
  types.insert(std::make_pair("uil", "text/x-uil"));
  types.insert(std::make_pair("uni", "text/uri-list"));
  types.insert(std::make_pair("unis", "text/uri-list"));
  types.insert(std::make_pair("unv", "application/i-deas"));
  types.insert(std::make_pair("uri", "text/uri-list"));
  types.insert(std::make_pair("uris", "text/uri-list"));
  types.insert(std::make_pair("ustar", "application/x-ustar"));
  types.insert(std::make_pair("uu", "text/x-uuencode"));
  types.insert(std::make_pair("uue", "text/x-uuencode"));
  types.insert(std::make_pair("vcd", "application/x-cdlink"));
  types.insert(std::make_pair("vcs", "text/x-vcalendar"));
  types.insert(std::make_pair("vda", "application/vda"));
  types.insert(std::make_pair("vdo", "video/vdo"));
  types.insert(std::make_pair("vew", "application/groupwise"));
  types.insert(std::make_pair("viv", "video/vivo"));
  types.insert(std::make_pair("vivo", "video/vivo"));
  types.insert(std::make_pair("vmd", "application/vocaltec-media-desc"));
  types.insert(std::make_pair("vmf", "application/vocaltec-media-file"));
  types.insert(std::make_pair("voc", "audio/voc"));
  types.insert(std::make_pair("vos", "video/vosaic"));
  types.insert(std::make_pair("vox", "audio/voxware"));
  types.insert(std::make_pair("vqe", "audio/x-twinvq-plugin"));
  types.insert(std::make_pair("vqf", "audio/x-twinvq"));
  types.insert(std::make_pair("vql", "audio/x-twinvq-plugin"));
  types.insert(std::make_pair("vrml", "application/x-vrml"));
  types.insert(std::make_pair("vrt", "x-world/x-vrt"));
  types.insert(std::make_pair("vsd", "application/x-visio"));
  types.insert(std::make_pair("vst", "application/x-visio"));
  types.insert(std::make_pair("vsw", "application/x-visio"));
  types.insert(std::make_pair("vtt", "text/vtt"));
  types.insert(std::make_pair("w60", "application/wordperfect6.0"));
  types.insert(std::make_pair("w61", "application/wordperfect6.1"));
  types.insert(std::make_pair("w6w", "application/msword"));
  types.insert(std::make_pair("wav", "audio/wav"));
  types.insert(std::make_pair("wb1", "application/x-qpro"));
  types.insert(std::make_pair("wbmp", "image/vnd.wap.wbmp"));
  types.insert(std::make_pair("web", "application/vnd.xara"));
  types.insert(std::make_pair("webp", "image/webp"));
  types.insert(std::make_pair("wiz", "application/msword"));
  types.insert(std::make_pair("wk1", "application/x-123"));
  types.insert(std::make_pair("wma", "audio/x-ms-wma"));
  types.insert(std::make_pair("wmf", "windows/metafile"));
  types.insert(std::make_pair("wml", "text/vnd.wap.wml"));
  types.insert(std::make_pair("wmlc", "application/vnd.wap.wmlc"));
  types.insert(std::make_pair("wmls", "text/vnd.wap.wmlscript"));
  types.insert(std::make_pair("wmlsc", "application/vnd.wap.wmlscriptc"));
  types.insert(std::make_pair("wmv", "video/x-ms-wmv"));
  types.insert(std::make_pair("word", "application/msword"));
  types.insert(std::make_pair("wp", "application/wordperfect"));
  types.insert(std::make_pair("wp5", "application/wordperfect"));
  types.insert(std::make_pair("wp6", "application/wordperfect"));
  types.insert(std::make_pair("wpd", "application/wordperfect"));
  types.insert(std::make_pair("wq1", "application/x-lotus"));
  types.insert(std::make_pair("wri", "application/mswrite"));
  types.insert(std::make_pair("wrl", "model/vrml"));
  types.insert(std::make_pair("wrz", "model/vrml"));
  types.insert(std::make_pair("wsc", "text/scriplet"));
  types.insert(std::make_pair("wsrc", "application/x-wais-source"));
  types.insert(std::make_pair("wtk", "application/x-wintalk"));
  types.insert(std::make_pair("x3f", "image/x3f"));
  types.insert(std::make_pair("xbm", "image/xbm"));
  types.insert(std::make_pair("xdr", "video/x-amt-demorun"));
  types.insert(std::make_pair("xgz", "xgl/drawing"));
  types.insert(std::make_pair("xif", "image/vnd.xiff"));
  types.insert(std::make_pair("xl", "application/excel"));
  types.insert(std::make_pair("xla", "application/excel"));
  types.insert(std::make_pair("xlb", "application/excel"));
  types.insert(std::make_pair("xlc", "application/excel"));
  types.insert(std::make_pair("xld", "application/excel"));
  types.insert(std::make_pair("xlk", "application/excel"));
  types.insert(std::make_pair("xll", "application/excel"));
  types.insert(std::make_pair("xlm", "application/excel"));
  types.insert(std::make_pair("xls", "application/excel"));
  types.insert(std::make_pair("xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"));
  types.insert(std::make_pair("xlt", "application/excel"));
  types.insert(std::make_pair("xlv", "application/excel"));
  types.insert(std::make_pair("xlw", "application/excel"));
  types.insert(std::make_pair("xm", "audio/xm"));
  types.insert(std::make_pair("xml", "text/xml"));
  types.insert(std::make_pair("xmz", "xgl/movie"));
  types.insert(std::make_pair("xpix", "application/x-vnd.ls-xpix"));
  types.insert(std::make_pair("xpm", "image/xpm"));
  types.insert(std::make_pair("x-png", "image/png"));
  types.insert(std::make_pair("xspf", "application/xspf+xml"));
  types.insert(std::make_pair("xsr", "video/x-amt-showrun"));
  types.insert(std::make_pair("xvid", "video/x-msvideo"));
  types.insert(std::make_pair("xwd", "image/x-xwd"));
  types.insert(std::make_pair("xyz", "chemical/x-pdb"));
  types.insert(std::make_pair("z", "application/x-compressed"));
  types.insert(std::make_pair("zip", "application/zip"));
  types.insert(std::make_pair("zoo", "application/octet-stream"));
  types.insert(std::make_pair("zsh", "text/x-script.zsh"));
#endif
  return types;
}
}

const std::map<std::string, std::string> CMime::m_mimetypes = CreateMimeTypes();

std::string CMime::GetMimeType(const std::string &extension)
{
  if (extension.empty())
    return "";

  std::string ext = extension;
  size_t posNotPoint = ext.find_first_not_of('.');
  if (posNotPoint != std::string::npos && posNotPoint > 0)
    ext = extension.substr(posNotPoint);
  transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  std::map<std::string, std::string>::const_iterator it = m_mimetypes.find(ext);
  if (it != m_mimetypes.end())
    return it->second;

  return "";
}

std::string CMime::GetMimeType(const CFileItem &item)
{
  std::string path = item.GetDynPath();
  if (item.HasVideoInfoTag() && !item.GetVideoInfoTag()->GetPath().empty())
    path = item.GetVideoInfoTag()->GetPath();
  else if (item.HasMusicInfoTag() && !item.GetMusicInfoTag()->GetURL().empty())
    path = item.GetMusicInfoTag()->GetURL();

  return GetMimeType(URIUtils::GetExtension(path));
}

std::string CMime::GetMimeType(const CURL &url, bool lookup)
{

  std::string strMimeType;

  if( url.IsProtocol("shout") || url.IsProtocol("http") || url.IsProtocol("https"))
  {
    // If lookup is false, bail out early to leave mime type empty
    if (!lookup)
      return strMimeType;

    std::string strmime;
    XFILE::CCurlFile::GetMimeType(url, strmime);

    // try to get mime-type again but with an NSPlayer User-Agent
    // in order for server to provide correct mime-type.  Allows us
    // to properly detect an MMS stream
    if (StringUtils::StartsWithNoCase(strmime, "video/x-ms-"))
      XFILE::CCurlFile::GetMimeType(url, strmime, "NSPlayer/11.00.6001.7000");

    // make sure there are no options set in mime-type
    // mime-type can look like "video/x-ms-asf ; charset=utf8"
    size_t i = strmime.find(';');
    if(i != std::string::npos)
      strmime.erase(i, strmime.length() - i);
    StringUtils::Trim(strmime);
    strMimeType = strmime;
  }
  else
    strMimeType = GetMimeType(url.GetFileType());

  // if it's still empty set to an unknown type
  if (strMimeType.empty())
    strMimeType = "application/octet-stream";

  return strMimeType;
}

CMime::EFileType CMime::GetFileTypeFromMime(const std::string& mimeType)
{
  // based on http://mimesniff.spec.whatwg.org/

  std::string type, subtype;
  if (!parseMimeType(mimeType, type, subtype))
    return FileTypeUnknown;

  if (type == "application")
  {
    if (subtype == "zip")
      return FileTypeZip;
    if (subtype == "x-gzip")
      return FileTypeGZip;
    if (subtype == "x-rar-compressed")
      return FileTypeRar;

    if (subtype == "xml")
      return FileTypeXml;
  }
  else if (type == "text")
  {
    if (subtype == "xml")
      return FileTypeXml;
    if (subtype == "html")
      return FileTypeHtml;
    if (subtype == "plain")
      return FileTypePlainText;
  }
  else if (type == "image")
  {
    if (subtype == "bmp")
      return FileTypeBmp;
    if (subtype == "gif")
      return FileTypeGif;
    if (subtype == "png")
      return FileTypePng;
    if (subtype == "jpeg" || subtype == "pjpeg")
      return FileTypeJpeg;
  }

  if (StringUtils::EndsWith(subtype, "+zip"))
    return FileTypeZip;
  if (StringUtils::EndsWith(subtype, "+xml"))
    return FileTypeXml;

  return FileTypeUnknown;
}

CMime::EFileType CMime::GetFileTypeFromContent(const std::string& fileContent)
{
  // based on http://mimesniff.spec.whatwg.org/#matching-a-mime-type-pattern

  const size_t len = fileContent.length();
  if (len < 2)
    return FileTypeUnknown;

  const unsigned char* const b = (const unsigned char*)fileContent.c_str();

  //! @todo add detection for text types

  // check image types
  if (b[0] == 'B' && b[1] == 'M')
    return FileTypeBmp;
  if (len >= 6 && b[0] == 'G' && b[1] == 'I' && b[2] == 'F' && b[3] == '8' && (b[4] == '7' || b[4] == '9') && b[5] == 'a')
    return FileTypeGif;
  if (len >= 8 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' && b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A)
    return FileTypePng;
  if (len >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF)
    return FileTypeJpeg;

  // check archive types
  if (len >= 3 && b[0] == 0x1F && b[1] == 0x8B && b[2] == 0x08)
    return FileTypeGZip;
  if (len >= 4 && b[0] == 'P' && b[1] == 'K' && b[2] == 0x03 && b[3] == 0x04)
    return FileTypeZip;
  if (len >= 7 && b[0] == 'R' && b[1] == 'a' && b[2] == 'r' && b[3] == ' ' && b[4] == 0x1A && b[5] == 0x07 && b[6] == 0x00)
    return FileTypeRar;

  //! @todo add detection for other types if required

  return FileTypeUnknown;
}

bool CMime::parseMimeType(const std::string& mimeType, std::string& type, std::string& subtype)
{
  static const char* const whitespaceChars = "\x09\x0A\x0C\x0D\x20"; // tab, LF, FF, CR and space

  type.clear();
  subtype.clear();

  const size_t slashPos = mimeType.find('/');
  if (slashPos == std::string::npos)
    return false;

  type.assign(mimeType, 0, slashPos);
  subtype.assign(mimeType, slashPos + 1, std::string::npos);

  const size_t semicolonPos = subtype.find(';');
  if (semicolonPos != std::string::npos)
    subtype.erase(semicolonPos);

  StringUtils::Trim(type, whitespaceChars);
  StringUtils::Trim(subtype, whitespaceChars);

  if (type.empty() || subtype.empty())
  {
    type.clear();
    subtype.clear();
    return false;
  }

  StringUtils::ToLower(type);
  StringUtils::ToLower(subtype);

  return true;
}
