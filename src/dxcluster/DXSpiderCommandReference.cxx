string dxspider_cmds = "\n\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n\
<html>\n\
<head>\n\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\n\
<title>DX Spider Command Reference</title>\n\
</head>\n\
<body>\n\
<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"en\"><span\n\
dir=\"auto\">DXSpider Command Reference</span></h1>\n\
<div id=\"bodyContent\">\n\
<div id=\"mw-content-text\" dir=\"ltr\" class=\"mw-content-ltr\"\n\
lang=\"en\">\n\
<div id=\"toc\" class=\"toc\">\n\
<div id=\"toctitle\">\n\
<h2>Contents</h2>\n\
</div>\n\
</div>\n\
</div>\n\
</div>\n\
<div id=\"toc\" class=\"toc\">\n\
<ul>\n\
<li class=\"toclevel-1 tocsection-1\"><a\n\
href=\"#ACCEPT\"><span class=\"tocnumber\">1</span> <span class=\"toctext\">ACCEPT</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-2\"><a\n\
href=\"#accept.2Fannounce\"><span class=\"tocnumber\">1.1</span> <span class=\"toctext\">accept/announce</span></a></li>\n\
<li class=\"toclevel-2 tocsection-3\"><a\n\
href=\"#accept.2Fspots\"><span class=\"tocnumber\">1.2</span> <span class=\"toctext\">accept/spots</span></a></li>\n\
<li class=\"toclevel-2 tocsection-4\"><a\n\
href=\"#accept.2Fwcy\"><span class=\"tocnumber\">1.3</span> <span class=\"toctext\">accept/wcy</span></a></li>\n\
<li class=\"toclevel-2 tocsection-5\"><a\n\
href=\"#accept.2Fwwv\"><span class=\"tocnumber\">1.4</span> <span class=\"toctext\">accept/wwv</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-6\"><a\n\
href=\"#ANNOUNCE\"><span class=\"tocnumber\">2</span> <span class=\"toctext\">ANNOUNCE</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-7\"><a\n\
href=\"#announce_2\"><span class=\"tocnumber\">2.1</span> <span class=\"toctext\">announce</span></a></li>\n\
<li class=\"toclevel-2 tocsection-8\"><a\n\
href=\"#announce_full\"><span class=\"tocnumber\">2.2</span> <span class=\"toctext\">announce\n\
  full</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-9\"><a\n\
href=\"#APROPOS\"><span class=\"tocnumber\">3</span> <span class=\"toctext\">APROPOS</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-10\"><a\n\
href=\"#apropos_2\"><span class=\"tocnumber\">3.1</span> <span class=\"toctext\">apropos</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-11\"><a\n\
href=\"#BLANK\"><span class=\"tocnumber\">4</span> <span class=\"toctext\">BLANK</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-12\"><a\n\
href=\"#blank_2\"><span class=\"tocnumber\">4.1</span> <span class=\"toctext\">blank</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-13\"><a\n\
href=\"#BYE\"><span class=\"tocnumber\">5</span> <span class=\"toctext\">BYE</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-14\"><a\n\
href=\"#bye_2\"><span class=\"tocnumber\">5.1</span> <span class=\"toctext\">bye</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-15\"><a\n\
href=\"#CHAT\"><span class=\"tocnumber\">6</span> <span class=\"toctext\">CHAT</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-16\"><a\n\
href=\"#chat_2\"><span class=\"tocnumber\">6.1</span> <span class=\"toctext\">chat</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-17\"><a\n\
href=\"#CLEAR\"><span class=\"tocnumber\">7</span> <span class=\"toctext\">CLEAR</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-18\"><a\n\
href=\"#clear.2Fannounce\"><span class=\"tocnumber\">7.1</span> <span class=\"toctext\">clear/announce</span></a></li>\n\
<li class=\"toclevel-2 tocsection-19\"><a\n\
href=\"#clear.2Froute\"><span class=\"tocnumber\">7.2</span> <span class=\"toctext\">clear/route</span></a></li>\n\
<li class=\"toclevel-2 tocsection-20\"><a\n\
href=\"#clear.2Fspots_.5B0-9.7Call.5D\"><span class=\"tocnumber\">7.3</span> <span class=\"toctext\">clear/spots\n\
  [0-9|all]</span></a></li>\n\
<li class=\"toclevel-2 tocsection-21\"><a\n\
href=\"#clear.2Fwcy\"><span class=\"tocnumber\">7.4</span> <span class=\"toctext\">clear/wcy</span></a></li>\n\
<li class=\"toclevel-2 tocsection-22\"><a\n\
href=\"#clear.2Fwwv\"><span class=\"tocnumber\">7.5</span> <span class=\"toctext\">clear/wwv</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-23\"><a\n\
href=\"#DATABASES\"><span class=\"tocnumber\">8</span> <span class=\"toctext\">DATABASES</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-24\"><a\n\
href=\"#dbavail\"><span class=\"tocnumber\">8.1</span> <span class=\"toctext\">dbavail</span></a></li>\n\
<li class=\"toclevel-2 tocsection-25\"><a\n\
href=\"#dbshow\"><span class=\"tocnumber\">8.2</span> <span class=\"toctext\">dbshow</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-26\"><a\n\
href=\"#MAIL\"><span class=\"tocnumber\">9</span> <span class=\"toctext\">MAIL</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-27\"><a\n\
href=\"#directory\"><span class=\"tocnumber\">9.1</span> <span class=\"toctext\">directory</span></a></li>\n\
<li class=\"toclevel-2 tocsection-28\"><a\n\
href=\"#directory_.3Cfrom.3E-.3Cto.3E\"><span class=\"tocnumber\">9.2</span> <span class=\"toctext\">directory\n\
  &lt;from&gt;-&lt;to&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-29\"><a\n\
href=\"#directory_.3Cnn.3E\"><span class=\"tocnumber\">9.3</span> <span class=\"toctext\">directory\n\
  &lt;nn&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-30\"><a\n\
href=\"#directory_all\"><span class=\"tocnumber\">9.4</span> <span class=\"toctext\">directory\n\
  all</span></a></li>\n\
<li class=\"toclevel-2 tocsection-31\"><a\n\
href=\"#directory_from_.3Ccall.3E\"><span class=\"tocnumber\">9.5</span> <span class=\"toctext\">directory\n\
  from &lt;call&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-32\"><a\n\
href=\"#directory_new\"><span class=\"tocnumber\">9.6</span> <span class=\"toctext\">directory\n\
  new</span></a></li>\n\
<li class=\"toclevel-2 tocsection-33\"><a\n\
href=\"#directory_own\"><span class=\"tocnumber\">9.7</span> <span class=\"toctext\">directory\n\
  own</span></a></li>\n\
<li class=\"toclevel-2 tocsection-34\"><a\n\
href=\"#directory_subject_.3Cstring.3E\"><span class=\"tocnumber\">9.8</span> <span class=\"toctext\">directory\n\
  subject &lt;string&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-35\"><a\n\
href=\"#directory_to_.3Ccall.3E\"><span class=\"tocnumber\">9.9</span> <span class=\"toctext\">directory\n\
  to &lt;call&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-36\"><a\n\
href=\"#DX\"><span class=\"tocnumber\">10</span> <span class=\"toctext\">DX</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-37\"><a\n\
href=\"#dx_.5Bby_.3Ccall.3E.5D_.3Cfreq.3E_.3Ccall.3E_.3Cremarks.3E\"><span class=\"tocnumber\">10.1</span> <span class=\"toctext\">dx\n\
  [by &lt;call&gt;] &lt;freq&gt; &lt;call&gt;\n\
  &lt;remarks&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-38\"><a\n\
href=\"#ECHO\"><span class=\"tocnumber\">11</span> <span class=\"toctext\">ECHO</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-39\"><a\n\
href=\"#echo_.3Cline.3E\"><span class=\"tocnumber\">11.1</span> <span class=\"toctext\">echo\n\
  &lt;line&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-40\"><a\n\
href=\"#FILTERING\"><span class=\"tocnumber\">12</span> <span class=\"toctext\">FILTERING</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-41\"><a\n\
href=\"#filtering...\"><span class=\"tocnumber\">12.1</span> <span class=\"toctext\">filtering...</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-42\"><a\n\
href=\"#HELP\"><span class=\"tocnumber\">13</span> <span class=\"toctext\">HELP</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-43\"><a\n\
href=\"#help_2\"><span class=\"tocnumber\">13.1</span> <span class=\"toctext\">help</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-44\"><a\n\
href=\"#JOIN\"><span class=\"tocnumber\">14</span> <span class=\"toctext\">JOIN</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-45\"><a\n\
href=\"#join_.3Cgroup.3E\"><span class=\"tocnumber\">14.1</span> <span class=\"toctext\">join\n\
  &lt;group&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-46\"><a\n\
href=\"#KILL\"><span class=\"tocnumber\">15</span> <span class=\"toctext\">KILL</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-47\"><a\n\
href=\"#kill_.3Cfrom_msgno.3E-.3Cto_msgno.3E\"><span class=\"tocnumber\">15.1</span> <span class=\"toctext\">kill\n\
  &lt;from msgno&gt;-&lt;to msgno&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-48\"><a\n\
href=\"#kill_.3Cmsgno.3E_.5B.3Cmsgno...5D\"><span class=\"tocnumber\">15.2</span> <span class=\"toctext\">kill\n\
  &lt;msgno&gt; [&lt;msgno..]</span></a></li>\n\
<li class=\"toclevel-2 tocsection-49\"><a\n\
href=\"#kill_.3Cmsgno.3E_.5B.3Cmsgno.3E_....5D\"><span class=\"tocnumber\">15.3</span> <span class=\"toctext\">kill\n\
  &lt;msgno&gt; [&lt;msgno&gt; ...]</span></a></li>\n\
<li class=\"toclevel-2 tocsection-50\"><a\n\
href=\"#kill_from_.3Cregex.3E\"><span class=\"tocnumber\">15.4</span> <span class=\"toctext\">kill\n\
  from &lt;regex&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-51\"><a\n\
href=\"#kill_to_.3Cregex.3E\"><span class=\"tocnumber\">15.5</span> <span class=\"toctext\">kill\n\
  to &lt;regex&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-52\"><a\n\
href=\"#LEAVE\"><span class=\"tocnumber\">16</span> <span class=\"toctext\">LEAVE</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-53\"><a\n\
href=\"#leave_.3Cgroup.3E\"><span class=\"tocnumber\">16.1</span> <span class=\"toctext\">leave\n\
  &lt;group&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-54\"><a\n\
href=\"#LINKS\"><span class=\"tocnumber\">17</span> <span class=\"toctext\">LINKS</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-55\"><a\n\
href=\"#links_2\"><span class=\"tocnumber\">17.1</span> <span class=\"toctext\">links</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-56\"><a\n\
href=\"#READ\"><span class=\"tocnumber\">18</span> <span class=\"toctext\">READ</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-57\"><a\n\
href=\"#read_2\"><span class=\"tocnumber\">18.1</span> <span class=\"toctext\">read</span></a></li>\n\
<li class=\"toclevel-2 tocsection-58\"><a\n\
href=\"#read_.3Cmsgno.3E\"><span class=\"tocnumber\">18.2</span> <span class=\"toctext\">read\n\
  &lt;msgno&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-59\"><a\n\
href=\"#REJECT\"><span class=\"tocnumber\">19</span> <span class=\"toctext\">REJECT</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-60\"><a\n\
href=\"#reject_2\"><span class=\"tocnumber\">19.1</span> <span class=\"toctext\">reject</span></a></li>\n\
<li class=\"toclevel-2 tocsection-61\"><a\n\
href=\"#reject.2Fannounce_.5B0-9.5D_.3Cpattern.3E\"><span class=\"tocnumber\">19.2</span> <span class=\"toctext\">reject/announce\n\
  [0-9] &lt;pattern&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-62\"><a\n\
href=\"#reject.2Fspots_.5B0-9.5D_.3Cpattern.3E\"><span class=\"tocnumber\">19.3</span> <span class=\"toctext\">reject/spots\n\
  [0-9] &lt;pattern&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-63\"><a\n\
href=\"#reject.2Fwcy_.5B0-9.5D_.3Cpattern.3E\"><span class=\"tocnumber\">19.4</span> <span class=\"toctext\">reject/wcy\n\
  [0-9] &lt;pattern&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-64\"><a\n\
href=\"#reject.2Fwwv_.5B0-9.5D_.3Cpattern.3E\"><span class=\"tocnumber\">19.5</span> <span class=\"toctext\">reject/wwv\n\
  [0-9] &lt;pattern&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-65\"><a\n\
href=\"#REPLY\"><span class=\"tocnumber\">20</span> <span class=\"toctext\">REPLY</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-66\"><a\n\
href=\"#reply_2\"><span class=\"tocnumber\">20.1</span> <span class=\"toctext\">reply</span></a></li>\n\
<li class=\"toclevel-2 tocsection-67\"><a\n\
href=\"#reply_.3Cmsgno.3E\"><span class=\"tocnumber\">20.2</span> <span class=\"toctext\">reply\n\
  &lt;msgno&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-68\"><a\n\
href=\"#reply_b_.3Cmsgno.3E\"><span class=\"tocnumber\">20.3</span> <span class=\"toctext\">reply\n\
  b &lt;msgno&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-69\"><a\n\
href=\"#reply_noprivate_.3Cmsgno.3E\"><span class=\"tocnumber\">20.4</span> <span class=\"toctext\">reply\n\
  noprivate &lt;msgno&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-70\"><a\n\
href=\"#reply_rr_.3Cmsgno.3E\"><span class=\"tocnumber\">20.5</span> <span class=\"toctext\">reply\n\
  rr &lt;msgno&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-71\"><a\n\
href=\"#SEND\"><span class=\"tocnumber\">21</span> <span class=\"toctext\">SEND</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-72\"><a\n\
href=\"#send_.3Ccall.3E_.5B.3Ccall.3E_....5D\"><span class=\"tocnumber\">21.1</span> <span class=\"toctext\">send\n\
  &lt;call&gt; [&lt;call&gt; ...]</span></a></li>\n\
<li class=\"toclevel-2 tocsection-73\"><a\n\
href=\"#send_copy_.3Cmsgno.3E_.3Ccall.3E\"><span class=\"tocnumber\">21.2</span> <span class=\"toctext\">send\n\
  copy &lt;msgno&gt; &lt;call&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-74\"><a\n\
href=\"#send_noprivate_.3Ccall.3E\"><span class=\"tocnumber\">21.3</span> <span class=\"toctext\">send\n\
  noprivate &lt;call&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-75\"><a\n\
href=\"#send_private_.3Ccall.3E\"><span class=\"tocnumber\">21.4</span> <span class=\"toctext\">send\n\
  private &lt;call&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-76\"><a\n\
href=\"#send_rr_.3Ccall.3E\"><span class=\"tocnumber\">21.5</span> <span class=\"toctext\">send\n\
  rr &lt;call&gt;</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-77\"><a\n\
href=\"#SET\"><span class=\"tocnumber\">22</span> <span class=\"toctext\">SET</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-78\"><a\n\
href=\"#set.2Faddress_.3Cyour_address.3E\"><span class=\"tocnumber\">22.1</span> <span class=\"toctext\">set/address\n\
  &lt;your address&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-79\"><a\n\
href=\"#set.2Fannounce\"><span class=\"tocnumber\">22.2</span> <span class=\"toctext\">set/announce</span></a></li>\n\
<li class=\"toclevel-2 tocsection-80\"><a\n\
href=\"#set.2Fanntalk\"><span class=\"tocnumber\">22.3</span> <span class=\"toctext\">set/anntalk</span></a></li>\n\
<li class=\"toclevel-2 tocsection-81\"><a\n\
href=\"#set.2Fbeep\"><span class=\"tocnumber\">22.4</span> <span class=\"toctext\">set/beep</span></a></li>\n\
<li class=\"toclevel-2 tocsection-82\"><a\n\
href=\"#set.2Fdx\"><span class=\"tocnumber\">22.5</span> <span class=\"toctext\">set/dx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-83\"><a\n\
href=\"#set.2Fdxcq\"><span class=\"tocnumber\">22.6</span> <span class=\"toctext\">set/dxcq</span></a></li>\n\
<li class=\"toclevel-2 tocsection-84\"><a\n\
href=\"#set.2Fdxgrid\"><span class=\"tocnumber\">22.7</span> <span class=\"toctext\">set/dxgrid</span></a></li>\n\
<li class=\"toclevel-2 tocsection-85\"><a\n\
href=\"#set.2Fdxitu\"><span class=\"tocnumber\">22.8</span> <span class=\"toctext\">set/dxitu</span></a></li>\n\
<li class=\"toclevel-2 tocsection-86\"><a\n\
href=\"#set.2Fecho\"><span class=\"tocnumber\">22.9</span> <span class=\"toctext\">set/echo</span></a></li>\n\
<li class=\"toclevel-2 tocsection-87\"><a\n\
href=\"#set.2Femail_.3Cemail.3E\"><span class=\"tocnumber\">22.10</span> <span class=\"toctext\">set/email\n\
  &lt;email&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-88\"><a\n\
href=\"#set.2Fhere\"><span class=\"tocnumber\">22.11</span> <span class=\"toctext\">set/here</span></a></li>\n\
<li class=\"toclevel-2 tocsection-89\"><a\n\
href=\"#set.2Fhomenode_.3Cnode.3E\"><span class=\"tocnumber\">22.12</span> <span class=\"toctext\">set/homenode\n\
  &lt;node&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-90\"><a\n\
href=\"#set.2Flanguage_.3Clang.3E\"><span class=\"tocnumber\">22.13</span> <span class=\"toctext\">set/language\n\
  &lt;lang&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-91\"><a\n\
href=\"#set.2Flocation_.3Clat_.26_long.3E\"><span class=\"tocnumber\">22.14</span> <span class=\"toctext\">set/location\n\
  &lt;lat &amp; long&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-92\"><a\n\
href=\"#set.2Flogininfo\"><span class=\"tocnumber\">22.15</span> <span class=\"toctext\">set/logininfo</span></a></li>\n\
<li class=\"toclevel-2 tocsection-93\"><a\n\
href=\"#set.2Fname_.3Cyour_name.3E\"><span class=\"tocnumber\">22.16</span> <span class=\"toctext\">set/name\n\
  &lt;your name&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-94\"><a\n\
href=\"#set.2Fpage_.3Clines_per_page.3E\"><span class=\"tocnumber\">22.17</span> <span class=\"toctext\">set/page\n\
  &lt;lines per page&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-95\"><a\n\
href=\"#set.2Fpassword\"><span class=\"tocnumber\">22.18</span> <span class=\"toctext\">set/password</span></a></li>\n\
<li class=\"toclevel-2 tocsection-96\"><a\n\
href=\"#set.2Fprompt_.3Cstring.3E\"><span class=\"tocnumber\">22.19</span> <span class=\"toctext\">set/prompt\n\
  &lt;string&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-97\"><a\n\
href=\"#set.2Fqra_.3Clocator.3E\"><span class=\"tocnumber\">22.20</span> <span class=\"toctext\">set/qra\n\
  &lt;locator&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-98\"><a\n\
href=\"#set.2Fqth_.3Cyour_qth.3E\"><span class=\"tocnumber\">22.21</span> <span class=\"toctext\">set/qth\n\
  &lt;your qth&gt;</span></a></li>\n\
<li class=\"toclevel-2 tocsection-99\"><a\n\
href=\"#set.2Fstartup\"><span class=\"tocnumber\">22.22</span> <span class=\"toctext\">set/startup</span></a></li>\n\
<li class=\"toclevel-2 tocsection-100\"><a\n\
href=\"#set.2Ftalk\"><span class=\"tocnumber\">22.23</span> <span class=\"toctext\">set/talk</span></a></li>\n\
<li class=\"toclevel-2 tocsection-101\"><a\n\
href=\"#set.2Fusstate\"><span class=\"tocnumber\">22.24</span> <span class=\"toctext\">set/usstate</span></a></li>\n\
<li class=\"toclevel-2 tocsection-102\"><a\n\
href=\"#set.2Fwcy\"><span class=\"tocnumber\">22.25</span> <span class=\"toctext\">set/wcy</span></a></li>\n\
<li class=\"toclevel-2 tocsection-103\"><a\n\
href=\"#set.2Fwwv\"><span class=\"tocnumber\">22.26</span> <span class=\"toctext\">set/wwv</span></a></li>\n\
<li class=\"toclevel-2 tocsection-104\"><a\n\
href=\"#set.2Fwx\"><span class=\"tocnumber\">22.27</span> <span class=\"toctext\">set/wx</span></a></li>\n\
</ul>\n\
</li>\n\
<li class=\"toclevel-1 tocsection-105\"><a\n\
href=\"#SHOW\"><span class=\"tocnumber\">23</span> <span class=\"toctext\">SHOW</span></a>\n\
<ul>\n\
<li class=\"toclevel-2 tocsection-106\"><a\n\
href=\"#show.2Fchat\"><span class=\"tocnumber\">23.1</span> <span class=\"toctext\">show/chat</span></a></li>\n\
<li class=\"toclevel-2 tocsection-107\"><a\n\
href=\"#show.2Fconfiguration\"><span class=\"tocnumber\">23.2</span> <span class=\"toctext\">show/configuration</span></a></li>\n\
<li class=\"toclevel-2 tocsection-108\"><a\n\
href=\"#show.2Fconfiguration.2Fnode\"><span class=\"tocnumber\">23.3</span> <span class=\"toctext\">show/configuration/node</span></a></li>\n\
<li class=\"toclevel-2 tocsection-109\"><a\n\
href=\"#show.2Fcontest\"><span class=\"tocnumber\">23.4</span> <span class=\"toctext\">show/contest</span></a></li>\n\
<li class=\"toclevel-2 tocsection-110\"><a\n\
href=\"#show.2Fdate\"><span class=\"tocnumber\">23.5</span> <span class=\"toctext\">show/date</span></a></li>\n\
<li class=\"toclevel-2 tocsection-111\"><a\n\
href=\"#show.2Fdb0sdx\"><span class=\"tocnumber\">23.6</span> <span class=\"toctext\">show/db0sdx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-112\"><a\n\
href=\"#show.2Fdx\"><span class=\"tocnumber\">23.7</span> <span class=\"toctext\">show/dx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-113\"><a\n\
href=\"#show.2Fdxcc\"><span class=\"tocnumber\">23.8</span> <span class=\"toctext\">show/dxcc</span></a></li>\n\
<li class=\"toclevel-2 tocsection-114\"><a\n\
href=\"#show.2Fdxqsl\"><span class=\"tocnumber\">23.9</span> <span class=\"toctext\">show/dxqsl</span></a></li>\n\
<li class=\"toclevel-2 tocsection-115\"><a\n\
href=\"#show.2Fdxstats\"><span class=\"tocnumber\">23.10</span> <span class=\"toctext\">show/dxstats</span></a></li>\n\
<li class=\"toclevel-2 tocsection-116\"><a\n\
href=\"#show.2Ffdx\"><span class=\"tocnumber\">23.11</span> <span class=\"toctext\">show/fdx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-117\"><a\n\
href=\"#show.2Ffiles\"><span class=\"tocnumber\">23.12</span> <span class=\"toctext\">show/files</span></a></li>\n\
<li class=\"toclevel-2 tocsection-118\"><a\n\
href=\"#show.2Ffilter\"><span class=\"tocnumber\">23.13</span> <span class=\"toctext\">show/filter</span></a></li>\n\
<li class=\"toclevel-2 tocsection-119\"><a\n\
href=\"#show.2Fhfstats\"><span class=\"tocnumber\">23.14</span> <span class=\"toctext\">show/hfstats</span></a></li>\n\
<li class=\"toclevel-2 tocsection-120\"><a\n\
href=\"#show.2Fhftable\"><span class=\"tocnumber\">23.15</span> <span class=\"toctext\">show/hftable</span></a></li>\n\
<li class=\"toclevel-2 tocsection-121\"><a\n\
href=\"#show.2Fmoon\"><span class=\"tocnumber\">23.16</span> <span class=\"toctext\">show/moon</span></a></li>\n\
<li class=\"toclevel-2 tocsection-122\"><a\n\
href=\"#show.2Fmuf\"><span class=\"tocnumber\">23.17</span> <span class=\"toctext\">show/muf</span></a></li>\n\
<li class=\"toclevel-2 tocsection-123\"><a\n\
href=\"#show.2Fmydx\"><span class=\"tocnumber\">23.18</span> <span class=\"toctext\">show/mydx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-124\"><a\n\
href=\"#show.2Fnewconfiguration\"><span class=\"tocnumber\">23.19</span> <span class=\"toctext\">show/newconfiguration</span></a></li>\n\
<li class=\"toclevel-2 tocsection-125\"><a\n\
href=\"#show.2Fnewconfiguration.2Fnode\"><span class=\"tocnumber\">23.20</span> <span class=\"toctext\">show/newconfiguration/node</span></a></li>\n\
<li class=\"toclevel-2 tocsection-126\"><a\n\
href=\"#show.2Fprefix\"><span class=\"tocnumber\">23.21</span> <span class=\"toctext\">show/prefix</span></a></li>\n\
<li class=\"toclevel-2 tocsection-127\"><a\n\
href=\"#show.2Fqra\"><span class=\"tocnumber\">23.22</span> <span class=\"toctext\">show/qra</span></a></li>\n\
<li class=\"toclevel-2 tocsection-128\"><a\n\
href=\"#show.2Fqrz\"><span class=\"tocnumber\">23.23</span> <span class=\"toctext\">show/qrz</span></a></li>\n\
<li class=\"toclevel-2 tocsection-129\"><a\n\
href=\"#show.2Froute\"><span class=\"tocnumber\">23.24</span> <span class=\"toctext\">show/route</span></a></li>\n\
<li class=\"toclevel-2 tocsection-130\"><a\n\
href=\"#show.2Fsatellite\"><span class=\"tocnumber\">23.25</span> <span class=\"toctext\">show/satellite</span></a></li>\n\
<li class=\"toclevel-2 tocsection-131\"><a\n\
href=\"#show.2Fstartup\"><span class=\"tocnumber\">23.26</span> <span class=\"toctext\">show/startup</span></a></li>\n\
<li class=\"toclevel-2 tocsection-132\"><a\n\
href=\"#show.2Fstation\"><span class=\"tocnumber\">23.27</span> <span class=\"toctext\">show/station</span></a></li>\n\
<li class=\"toclevel-2 tocsection-133\"><a\n\
href=\"#show.2Fsun\"><span class=\"tocnumber\">23.28</span> <span class=\"toctext\">show/sun</span></a></li>\n\
<li class=\"toclevel-2 tocsection-134\"><a\n\
href=\"#show.2Ftime\"><span class=\"tocnumber\">23.29</span> <span class=\"toctext\">show/time</span></a></li>\n\
<li class=\"toclevel-2 tocsection-135\"><a\n\
href=\"#show.2Fusdb\"><span class=\"tocnumber\">23.30</span> <span class=\"toctext\">show/usdb</span></a></li>\n\
<li class=\"toclevel-2 tocsection-136\"><a\n\
href=\"#show.2Fvhfstats\"><span class=\"tocnumber\">23.31</span> <span class=\"toctext\">show/vhfstats</span></a></li>\n\
<li class=\"toclevel-2 tocsection-137\"><a\n\
href=\"#show.2Fvhftable\"><span class=\"tocnumber\">23.32</span> <span class=\"toctext\">show/vhftable</span></a></li>\n\
<li class=\"toclevel-2 tocsection-138\"><a\n\
href=\"#show.2Fwcy\"><span class=\"tocnumber\">23.33</span> <span class=\"toctext\">show/wcy</span></a></li>\n\
<li class=\"toclevel-2 tocsection-139\"><a\n\
href=\"#show.2Fwm7d\"><span class=\"tocnumber\">23.34</span> <span class=\"toctext\">show/wm7d</span></a></li>\n\
<li class=\"toclevel-2 tocsection-140\"><a\n\
href=\"#show.2Fwwv\"><span class=\"tocnumber\">23.35</span> <span class=\"toctext\">show/wwv</span></a></li>\n\
<li class=\"toclevel-2 tocsection-141\"><a\n\
href=\"#sysop\"><span class=\"tocnumber\">23.36</span> <span class=\"toctext\">sysop</span></a></li>\n\
<li class=\"toclevel-2 tocsection-142\"><a\n\
href=\"#talk\"><span class=\"tocnumber\">23.37</span> <span class=\"toctext\">talk</span></a></li>\n\
<li class=\"toclevel-2 tocsection-143\"><a\n\
href=\"#type\"><span class=\"tocnumber\">23.38</span> <span class=\"toctext\">type</span></a></li>\n\
<li class=\"toclevel-2 tocsection-144\"><a\n\
href=\"#unset.2Fannounce\"><span class=\"tocnumber\">23.39</span> <span class=\"toctext\">unset/announce</span></a></li>\n\
<li class=\"toclevel-2 tocsection-145\"><a\n\
href=\"#unset.2Fanntalk\"><span class=\"tocnumber\">23.40</span> <span class=\"toctext\">unset/anntalk</span></a></li>\n\
<li class=\"toclevel-2 tocsection-146\"><a\n\
href=\"#unset.2Fbeep\"><span class=\"tocnumber\">23.41</span> <span class=\"toctext\">unset/beep</span></a></li>\n\
<li class=\"toclevel-2 tocsection-147\"><a\n\
href=\"#unset.2Fdx\"><span class=\"tocnumber\">23.42</span> <span class=\"toctext\">unset/dx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-148\"><a\n\
href=\"#unset.2Fdxcq\"><span class=\"tocnumber\">23.43</span> <span class=\"toctext\">unset/dxcq</span></a></li>\n\
<li class=\"toclevel-2 tocsection-149\"><a\n\
href=\"#unset.2Fdxgrid\"><span class=\"tocnumber\">23.44</span> <span class=\"toctext\">unset/dxgrid</span></a></li>\n\
<li class=\"toclevel-2 tocsection-150\"><a\n\
href=\"#unset.2Fdxitu\"><span class=\"tocnumber\">23.45</span> <span class=\"toctext\">unset/dxitu</span></a></li>\n\
<li class=\"toclevel-2 tocsection-151\"><a\n\
href=\"#unset.2Fecho\"><span class=\"tocnumber\">23.46</span> <span class=\"toctext\">unset/echo</span></a></li>\n\
<li class=\"toclevel-2 tocsection-152\"><a\n\
href=\"#unset.2Femail\"><span class=\"tocnumber\">23.47</span> <span class=\"toctext\">unset/email</span></a></li>\n\
<li class=\"toclevel-2 tocsection-153\"><a\n\
href=\"#unset.2Fhere\"><span class=\"tocnumber\">23.48</span> <span class=\"toctext\">unset/here</span></a></li>\n\
<li class=\"toclevel-2 tocsection-154\"><a\n\
href=\"#unset.2Flogininfo\"><span class=\"tocnumber\">23.49</span> <span class=\"toctext\">unset/logininfo</span></a></li>\n\
<li class=\"toclevel-2 tocsection-155\"><a\n\
href=\"#unset.2Fprivilege\"><span class=\"tocnumber\">23.50</span> <span class=\"toctext\">unset/privilege</span></a></li>\n\
<li class=\"toclevel-2 tocsection-156\"><a\n\
href=\"#unset.2Fprompt\"><span class=\"tocnumber\">23.51</span> <span class=\"toctext\">unset/prompt</span></a></li>\n\
<li class=\"toclevel-2 tocsection-157\"><a\n\
href=\"#unset.2Fstartup\"><span class=\"tocnumber\">23.52</span> <span class=\"toctext\">unset/startup</span></a></li>\n\
<li class=\"toclevel-2 tocsection-158\"><a\n\
href=\"#unset.2Ftalk\"><span class=\"tocnumber\">23.53</span> <span class=\"toctext\">unset/talk</span></a></li>\n\
<li class=\"toclevel-2 tocsection-159\"><a\n\
href=\"#unset.2Fusstate\"><span class=\"tocnumber\">23.54</span> <span class=\"toctext\">unset/usstate</span></a></li>\n\
<li class=\"toclevel-2 tocsection-160\"><a\n\
href=\"#unset.2Fwcy\"><span class=\"tocnumber\">23.55</span> <span class=\"toctext\">unset/wcy</span></a></li>\n\
<li class=\"toclevel-2 tocsection-161\"><a\n\
href=\"#unset.2Fwwv\"><span class=\"tocnumber\">23.56</span> <span class=\"toctext\">unset/wwv</span></a></li>\n\
<li class=\"toclevel-2 tocsection-162\"><a\n\
href=\"#unset.2Fwx\"><span class=\"tocnumber\">23.57</span> <span class=\"toctext\">unset/wx</span></a></li>\n\
<li class=\"toclevel-2 tocsection-163\"><a\n\
href=\"#who\"><span class=\"tocnumber\">23.58</span> <span class=\"toctext\">who</span></a></li>\n\
<li class=\"toclevel-2 tocsection-164\"><a\n\
href=\"#wx\"><span class=\"tocnumber\">23.59</span> <span class=\"toctext\">wx</span></a></li>\n\
</ul>\n\
</li>\n\
</ul>\n\
</div>\n\
<h2><span class=\"mw-headline\" id=\"ACCEPT\">ACCEPT</span></h2>\n\
<ul>\n\
<li>accept - Set a filter to accept something\n\
</li>\n\
</ul>\n\
<p>There are 2 types of filter, accept and reject. See HELP\n\
  FILTERING for more info.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"accept.2Fannounce\">accept/announce</span></h3>\n\
<ul>\n\
<li>accept/announce [0-9] &lt;pattern&gt; Set an 'accept' filter\n\
line for announce\n\
</li>\n\
</ul>\n\
<p>Create an 'accept this announce' line for a filter.\n\
</p>\n\
<p>An accept filter line means that if the announce matches this\n\
  filter it is passed onto the user. See HELP FILTERING for more\n\
  info. Please read this to understand how filters work - it will\n\
  save a lot of grief later on.\n\
  You can use any of the following things in this line:-\n\
</p>\n\
<pre>info &lt;string&gt;            eg: iota or qsl\n\
by &lt;prefixes&gt;            eg: G,M,2\n\
origin &lt;prefixes&gt;\n\
origin_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
origin_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
origin_zone &lt;prefixes or numbers&gt;\n\
origin_state &lt;states&gt;                eg: VA,NH,RI,NH\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
by_state &lt;states&gt;\n\
channel &lt;prefixes&gt;\n\
wx 1                     filter WX announces\n\
dest &lt;prefixes&gt;          eg: 6MUK,WDX      (distros)\n\
</pre>\n\
<p>some examples:-\n\
</p>\n\
<pre>acc/ann dest 6MUK\n\
acc/ann 2 by_zone 14,15,16\n\
(this could be all on one line: acc/ann dest 6MUK or by_zone 14,15,16)\n\
</pre>\n\
<p>or\n\
</p>\n\
<pre>acc/ann by G,M,2\n\
</pre>\n\
<p>for american states\n\
</p>\n\
<pre>acc/ann by_state va,nh,ri,nh\n\
</pre>\n\
<p>You can use the tag 'all' to accept everything eg:\n\
</p>\n\
<pre>acc/ann all\n\
</pre>\n\
<p>but this probably for advanced users...\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"accept.2Fspots\">accept/spots</span></h3>\n\
<ul>\n\
<li>accept/spots [0-9] &lt;pattern&gt; Set an 'accept' filter line\n\
for spots\n\
</li>\n\
</ul>\n\
<p>Create an 'accept this spot' line for a filter.\n\
</p>\n\
<p>An accept filter line means that if the spot matches this filter\n\
  it is passed onto the user. See HELP FILTERING for more info.\n\
  Please read this to understand how filters work - it will save a\n\
  lot of grief later on.\n\
</p>\n\
<p>You can use any of the following things in this line:-\n\
</p>\n\
<pre>freq &lt;range&gt;           eg: 0/30000 or hf or hf/cw or 6m,4m,2m\n\
on &lt;range&gt;             same as 'freq'\n\
call &lt;prefixes&gt;        eg: G,PA,HB9\n\
info &lt;string&gt;          eg: iota or qsl\n\
by &lt;prefixes&gt;\n\
call_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
call_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
call_zone &lt;prefixes or numbers&gt;\n\
call_state &lt;states&gt;                eg: VA,NH,RI,ME\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
by_state &lt;states&gt;                eg: VA,NH,RI,ME\n\
origin &lt;prefixes&gt;\n\
channel &lt;prefixes&gt;\n\
</pre>\n\
<p>For frequencies, you can use any of the band names defined in\n\
  SHOW/BANDS and you can use a subband name like: cw, rtty, data,\n\
  ssb - thus: hf/ssb. You can also just have a simple range like:\n\
  0/30000 - this is more efficient than saying simply: freq HF (but\n\
  don't get too hung up about that)\n\
</p>\n\
<p>some examples:-\n\
</p>\n\
<pre>acc/spot 1 on hf/cw\n\
acc/spot 2 on vhf and (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>You can use the tag 'all' to accept everything, eg:\n\
</p>\n\
<pre>acc/spot 3 all\n\
</pre>\n\
<p>for US states\n\
</p>\n\
<pre>acc/spots by_state VA,NH,RI,MA,ME\n\
</pre>\n\
<p>but this probably for advanced users...\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"accept.2Fwcy\">accept/wcy</span></h3>\n\
<ul>\n\
<li>accept/wcy [0-9] &lt;pattern&gt; set an 'accept' WCY filter\n\
</li>\n\
</ul>\n\
<p>It is unlikely that you will want to do this, but if you do then\n\
  you can filter on the following fields:-\n\
</p>\n\
<pre>by &lt;prefixes&gt;            eg: G,M,2\n\
origin &lt;prefixes&gt;\n\
origin_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
origin_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
origin_zone &lt;prefixes or numbers&gt;\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
channel &lt;prefixes&gt;\n\
</pre>\n\
<p>There are no examples because WCY Broadcasts only come from one\n\
  place and you either want them or not (see UNSET/WCY if you don't\n\
  want them).\n\
</p>\n\
<p>This command is really provided for future use.\n\
</p>\n\
<p>See HELP FILTER for information.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"accept.2Fwwv\">accept/wwv</span></h3>\n\
<ul>\n\
<li>accept/wwv [0-9] &lt;pattern&gt; set an 'accept' WWV filter\n\
</li>\n\
</ul>\n\
<p>It is unlikely that you will want to do this, but if you do then\n\
  you can filter on the following fields:-\n\
</p>\n\
<pre>by &lt;prefixes&gt;            eg: G,M,2\n\
origin &lt;prefixes&gt;\n\
origin_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
origin_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
origin_zone &lt;prefixes or numbers&gt;\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
channel &lt;prefixes&gt;\n\
</pre>\n\
<p>for example\n\
</p>\n\
<pre>accept/wwv by_zone 4\n\
</pre>\n\
<p>is probably the only useful thing to do (which will only show WWV\n\
  broadcasts by stations in the US).\n\
</p>\n\
<p>See HELP FILTER for information.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"ANNOUNCE\">ANNOUNCE</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"announce_2\">announce</span></h3>\n\
<ul>\n\
<li>announce &lt;text&gt; Send an announcement to LOCAL users only\n\
</li>\n\
</ul>\n\
<p>&lt;text&gt; is the text of the announcement you wish to\n\
  broadcast\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"announce_full\">announce full</span></h3>\n\
<ul>\n\
<li>announce full &lt;text&gt; Send an announcement cluster wide\n\
</li>\n\
</ul>\n\
<p>This will send your announcement cluster wide\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"APROPOS\">APROPOS</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"apropos_2\">apropos</span></h3>\n\
<ul>\n\
<li>apropos &lt;string&gt; Search help database for &lt;string&gt;\n\
</li>\n\
</ul>\n\
<p>Search the help database for &lt;string&gt; (it isn't case\n\
  sensitive), and print the names of all the commands that may be\n\
  relevant.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"BLANK\">BLANK</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"blank_2\">blank</span></h3>\n\
<ul>\n\
<li>blank [&lt;string&gt;] [&lt;nn&gt;] Print nn (default 1) blank\n\
lines (or strings)\n\
</li>\n\
</ul>\n\
<p>In its basic form this command prints one or more blank lines.\n\
  However if you pass it a string it will replicate the string for\n\
  the width of the screen (default 80) and then print that one or\n\
  more times, so:\n\
</p>\n\
<pre>blank 2\n\
</pre>\n\
<p>prints two blank lines\n\
</p>\n\
<pre>blank -\n\
</pre>\n\
<p>prints a row of - characters once.\n\
</p>\n\
<pre>blank abc\n\
</pre>\n\
<p>prints 'abcabcabcabcabcabc....'\n\
</p>\n\
<p>This is really only of any use in a script file and you can print\n\
  a maximum of 9 lines.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"BYE\">BYE</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"bye_2\">bye</span></h3>\n\
<ul>\n\
<li>bye Exit from the cluster\n\
</li>\n\
</ul>\n\
<p>This will disconnect you from the cluster\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"CHAT\">CHAT</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"chat_2\">chat</span></h3>\n\
<ul>\n\
<li>chat &lt;group&gt; &lt;text&gt; Chat or Conference to a group\n\
</li>\n\
</ul>\n\
<p>It is now possible to JOIN a group and have network wide\n\
  conferencing to that group. DXSpider does not (and probably will\n\
  not) implement the AK1A conference mode as this seems very\n\
  limiting, is hardly used and doesn't seem to work too well anyway.\n\
</p>\n\
<p>This system uses the existing ANN system and is compatible with\n\
  both other DXSpider nodes and AK1A clusters (they use\n\
  ANN/&lt;group&gt;).\n\
</p>\n\
<p>You can be a member of as many \"groups\" as you want. To join a\n\
  group type:-\n\
</p>\n\
<pre>JOIN FOC    (where FOC is the group name)\n\
</pre>\n\
<p>To leave a group type:-\n\
</p>\n\
<pre>LEAVE FOC\n\
</pre>\n\
<p>You can see which groups you are in by typing:-\n\
</p>\n\
<pre>STAT/USER\n\
</pre>\n\
<p>and you can see whether your mate is in the group, if he connects\n\
  to the same node as you, by typing:-\n\
</p>\n\
<pre>STAT/USER g1tlh\n\
</pre>\n\
<p>To send a message to a group type:-\n\
</p>\n\
<pre>CHAT FOC hello everyone\n\
</pre>\n\
<p>or\n\
</p>\n\
<pre>CH #9000 hello I am back\n\
</pre>\n\
<p>See also JOIN, LEAVE, SHOW/CHAT\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"CLEAR\">CLEAR</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"clear.2Fannounce\">clear/announce</span></h3>\n\
<ul>\n\
<li>clear/announce [1|all] Clear a announce filter line\n\
</li>\n\
</ul>\n\
<p>This command allows you to clear (remove) a line in a annouce\n\
  filter or to remove the whole filter. See CLEAR/SPOTS for a more\n\
  detailed explanation.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"clear.2Froute\">clear/route</span></h3>\n\
<ul>\n\
<li>clear/route [1|all] Clear a route filter line\n\
</li>\n\
</ul>\n\
<p>This command allows you to clear (remove) a line in a route\n\
  filter or to remove the whole filter. See CLEAR/SPOTS for a more\n\
  detailed explanation.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"clear.2Fspots_.5B0-9.7Call.5D\">clear/spots\n\
[0-9|all]</span></h3>\n\
<ul>\n\
<li>clear/spots [0-9|all] Clear a spot filter line\n\
</li>\n\
</ul>\n\
<p>This command allows you to clear (remove) a line in a spot filter\n\
  or to remove the whole filter.\n\
</p>\n\
<p>If you have a filter:-\n\
</p>\n\
<pre>acc/spot 1 on hf/cw\n\
acc/spot 2 on vhf and (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>and you say:-\n\
</p>\n\
<pre>clear/spot 1\n\
</pre>\n\
<p>you will be left with:-\n\
</p>\n\
<pre>acc/spot 2 on vhf and (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>If you do:\n\
</p>\n\
<pre>clear/spot all\n\
</pre>\n\
<p>the filter will be completely removed.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"clear.2Fwcy\">clear/wcy</span></h3>\n\
<ul>\n\
<li>clear/wcy [1|all] Clear a WCY filter line\n\
</li>\n\
</ul>\n\
<p>This command allows you to clear (remove) a line in a WCY filter\n\
  or to remove the whole filter. See CLEAR/SPOTS for a more detailed\n\
  explanation.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"clear.2Fwwv\">clear/wwv</span></h3>\n\
<ul>\n\
<li>clear/wwv [1|all] Clear a WWV filter line\n\
</li>\n\
</ul>\n\
<p>This command allows you to clear (remove) a line in a WWV filter\n\
  or to remove the whole filter. See CLEAR/SPOTS for a more detailed\n\
  explanation.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"DATABASES\">DATABASES</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"dbavail\">dbavail</span></h3>\n\
<ul>\n\
<li>dbavail - Show a list of all the Databases in the system\n\
</li>\n\
</ul>\n\
<p>The name says it all really, this command lists all the databases\n\
  defined in the system. It is also aliased to SHOW/COMMAND.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"dbshow\">dbshow</span></h3>\n\
<ul>\n\
<li>dbshow &lt;dbname&gt; &lt;key&gt; - Display an entry, if it\n\
exists, in a database\n\
</li>\n\
</ul>\n\
<p>This is the generic user interface to the database to the\n\
  database system. It is expected that the sysop will add an entry\n\
  to the local Aliases file so that users can use the more familiar\n\
  AK1A style of enquiry such as:\n\
</p>\n\
<pre>SH/BUCK G1TLH\n\
</pre>\n\
<p>but if he hasn't and the database really does exist (use DBAVAIL\n\
  or SHOW/COMMAND to find out) you can do the same thing with:\n\
</p>\n\
<pre>DBSHOW buck G1TLH\n\
</pre>\n\
<h2><span class=\"mw-headline\" id=\"MAIL\">MAIL</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"directory\">directory</span></h3>\n\
<ul>\n\
<li>directory - List messages\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_.3Cfrom.3E-.3Cto.3E\">directory\n\
&lt;from&gt;-&lt;to&gt;</span></h3>\n\
<ul>\n\
<li>directory &lt;from&gt;-&lt;to&gt; - List messages &lt;from&gt;\n\
message &lt;to&gt; message\n\
</li>\n\
</ul>\n\
<p>List the messages in the messages directory.\n\
</p>\n\
<p>If there is a 'p' one space after the message number then it is a\n\
  personal message. If there is a '-' between the message number and\n\
  the 'p' then this indicates the message has been read.\n\
</p>\n\
<p>You can use shell escape characters such as '*' and '?' in the\n\
  &lt;call&gt; fields.\n\
</p>\n\
<p>You can also combine some of the various directory commands\n\
  together eg:-\n\
</p>\n\
<pre>DIR TO G1TLH 5\n\
</pre>\n\
<p>or\n\
</p>\n\
<pre>DIR SUBJECT IOTA 200-250\n\
</pre>\n\
<p>You can abbreviate all the commands to one letter and use ak1a\n\
  syntax:-\n\
</p>\n\
<pre>DIR/T G1* 10\n\
DIR/S QSL 10-100 5\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"directory_.3Cnn.3E\">directory\n\
&lt;nn&gt;</span></h3>\n\
<ul>\n\
<li>directory &lt;nn&gt; - List last &lt;nn&gt; messages\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_all\">directory all</span></h3>\n\
<ul>\n\
<li>directory all - List all messages\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_from_.3Ccall.3E\">directory\n\
from &lt;call&gt;</span></h3>\n\
<ul>\n\
<li>directory from &lt;call&gt; - List all messages from\n\
&lt;call&gt;\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_new\">directory new</span></h3>\n\
<ul>\n\
<li>directory new - List all new messages\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_own\">directory own</span></h3>\n\
<ul>\n\
<li>directory own - List your own messages\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_subject_.3Cstring.3E\">directory\n\
subject &lt;string&gt;</span></h3>\n\
<ul>\n\
<li>directory subject &lt;string&gt; - List all messages with\n\
&lt;string&gt; in subject\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"directory_to_.3Ccall.3E\">directory\n\
to &lt;call&gt;</span></h3>\n\
<ul>\n\
<li>directory to &lt;call&gt; - List all messages to &lt;call&gt;\n\
</li>\n\
</ul>\n\
<h2><span class=\"mw-headline\" id=\"DX\">DX</span></h2>\n\
<h3><span class=\"mw-headline\"\n\
id=\"dx_.5Bby_.3Ccall.3E.5D_.3Cfreq.3E_.3Ccall.3E_.3Cremarks.3E\">dx\n\
[by &lt;call&gt;] &lt;freq&gt; &lt;call&gt; &lt;remarks&gt;</span></h3>\n\
<ul>\n\
<li>dx [by &lt;call&gt;] &lt;freq&gt; &lt;call&gt; &lt;remarks&gt;\n\
- Send a DX spot\n\
</li>\n\
</ul>\n\
<p>This is how you send a DX Spot to other users. You can, in fact,\n\
  now enter the &lt;freq&gt; and the &lt;call&gt; either way round.\n\
</p>\n\
<pre>DX FR0G 144.600\n\
DX 144.600 FR0G\n\
DX 144600 FR0G\n\
</pre>\n\
<p>will all give the same result. You can add some remarks to the\n\
  end of the command and they will be added to the spot.\n\
</p>\n\
<pre>DX FR0G 144600 this is a test\n\
</pre>\n\
<p>You can credit someone else by saying:-\n\
</p>\n\
<pre>DX by G1TLH FR0G 144.600 he isn't on the cluster\n\
</pre>\n\
<p>The &lt;freq&gt; is compared against the available bands set up\n\
  in the cluster. See SHOW/BANDS for more information.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"ECHO\">ECHO</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"echo_.3Cline.3E\">echo &lt;line&gt;</span></h3>\n\
<ul>\n\
<li>echo &lt;line&gt; - Echo the line to the output\n\
</li>\n\
</ul>\n\
<p>This command is useful in scripts and so forth for printing the\n\
  line that you give to the command to the output. You can use this\n\
  in user_default scripts and the SAVE command for titling and so\n\
  forth.\n\
</p>\n\
<p>The script will interpret certain standard \"escape\" sequences as\n\
  follows:-\n\
</p>\n\
<pre>\\t - becomes a TAB character (0x09 in ascii)\n\
\\a - becomes a BEEP character (0x07 in ascii)\n\
\\n - prints a new line\n\
</pre>\n\
<p>So the following example:-\n\
</p>\n\
<pre>echo GB7DJK is a dxcluster\n\
</pre>\n\
<p>produces:-\n\
</p>\n\
<pre>GB7DJK is a dxcluster\n\
</pre>\n\
<p>on the output. You don't need a \\n on the end of the line you\n\
  want to send.\n\
</p>\n\
<p>A more complex example:-\n\
</p>\n\
<pre>echo GB7DJK\\n\\tg1tlh\\tDirk\\n\\tg3xvf\\tRichard\n\
</pre>\n\
<p>produces:-\n\
</p>\n\
<pre>GB7DJK\n\
g1tlh   Dirk\n\
g3xvf   Richard\n\
</pre>\n\
<p>on the output.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"FILTERING\">FILTERING</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"filtering...\">filtering...</span></h3>\n\
<ul>\n\
<li>filtering... - Filtering things in DXSpider\n\
</li>\n\
</ul>\n\
<p>There are a number of things you can filter in the DXSpider\n\
  system. They all use the same general mechanism.\n\
</p>\n\
<p>In general terms you can create a 'reject' or an 'accept' filter\n\
  which can have up to 10 lines in it. You do this using, for\n\
  example:-\n\
</p>\n\
<pre>accept/spots .....\n\
reject/spots .....\n\
</pre>\n\
<p>where ..... are the specific commands for that type of filter.\n\
  There are filters for spots, wwv, announce, wcy and (for sysops)\n\
  connects. See each different accept or reject command reference\n\
  for more details.\n\
</p>\n\
<p>There is also a command to clear out one or more lines in a\n\
  filter and one to show you what you have set. They are:-\n\
</p>\n\
<pre>clear/spots 1\n\
clear/spots all\n\
</pre>\n\
<p>and\n\
</p>\n\
<pre>show/filter\n\
</pre>\n\
<p>There is clear/xxxx command for each type of filter.\n\
</p>\n\
<p>For now we are going to use spots for the examples, but you can\n\
  apply the principles to all types of filter.\n\
</p>\n\
<p>There are two main types of filter 'accept' or 'reject'; which\n\
  you use depends entirely on how you look at the world and what is\n\
  least writing to achieve what you want. Each filter has 10 lines\n\
  (of any length) which are tried in order. If a line matches then\n\
  the action you have specified is taken (ie reject means ignore it\n\
  and accept means gimme it).\n\
</p>\n\
<p>The important thing to remember is that if you specify a 'reject'\n\
  filter (all the lines in it say 'reject/spots' (for instance))\n\
  then if a spot comes in that doesn't match any of the lines then\n\
  you will get it BUT if you specify an 'accept' filter then any\n\
  spots that don't match are dumped. For example if I have a one\n\
  line accept filter:-\n\
</p>\n\
<pre>accept/spots on vhf and (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>then automatically you will ONLY get VHF spots from or to CQ\n\
  zones 14 15 and 16. If you set a reject filter like:\n\
</p>\n\
<pre>reject/spots on hf/cw\n\
</pre>\n\
<p>Then you will get everything EXCEPT HF CW spots, If you am\n\
  interested in IOTA and will work it even on CW then you could\n\
  say:-\n\
</p>\n\
<pre>reject/spots on hf/cw and not info iota\n\
</pre>\n\
<p>But in that case you might only be interested in iota and say:-\n\
</p>\n\
<pre>accept/spots not on hf/cw or info iota\n\
</pre>\n\
<p>which is exactly the same. You should choose one or the other\n\
  until you are confortable with the way it works. Yes, you can mix\n\
  them (actually you can have an accept AND a reject on the same\n\
  line) but don't try this at home until you can analyse the results\n\
  that you get without ringing up the sysop for help.\n\
</p>\n\
<p>Another useful addition now is filtering by US state\n\
</p>\n\
<pre>accept/spots by_state VA,NH,RI,ME\n\
</pre>\n\
<p>You can arrange your filter lines into logical units, either for\n\
  your own understanding or simply convenience. I have one set\n\
  frequently:-\n\
</p>\n\
<pre>reject/spots 1 on hf/cw\n\
reject/spots 2 on 50000/1400000 not (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>What this does is to ignore all HF CW spots (being a class B I\n\
  can't read any CW and couldn't possibly be interested in\n\
  HF&nbsp;:-) and also rejects any spots on VHF which don't either\n\
  originate or spot someone in Europe.\n\
</p>\n\
<p>This is an exmaple where you would use the line number (1 and 2\n\
  in this case), if you leave the digit out, the system assumes '1'.\n\
  Digits '0'-'9' are available.\n\
</p>\n\
<p>You can leave the word 'and' out if you want, it is implied. You\n\
  can use any number of brackets to make the 'expression' as you\n\
  want it. There are things called precedence rules working here\n\
  which mean that you will NEED brackets in a situation like line 2\n\
  because, without it, will assume:-\n\
</p>\n\
<pre>(on 50000/1400000 and by_zone 14,15,16) or call_zone 14,15,16\n\
</pre>\n\
<p>annoying, but that is the way it is. If you use OR - use\n\
  brackets. Whilst we are here CASE is not important. 'And BY_Zone'\n\
  is just 'and by_zone'.\n\
</p>\n\
<p>If you want to alter your filter you can just redefine one or\n\
  more lines of it or clear out one line. For example:-\n\
</p>\n\
<pre>reject/spots 1 on hf/ssb\n\
</pre>\n\
<p>or\n\
</p>\n\
<pre>clear/spots 1\n\
</pre>\n\
<p>To remove the filter in its entirety:-\n\
</p>\n\
<pre>clear/spots all\n\
</pre>\n\
<p>There are similar CLEAR commands for the other filters:-\n\
</p>\n\
<pre>clear/announce\n\
clear/wcy\n\
clear/wwv\n\
</pre>\n\
<p>ADVANCED USERS:-\n\
</p>\n\
<p>Once you are happy with the results you get, you may like to\n\
  experiment.\n\
</p>\n\
<p>My example that filters hf/cw spots and accepts vhf/uhf spots\n\
  from EU can be written with a mixed filter, eg:\n\
</p>\n\
<pre>rej/spot on hf/cw\n\
acc/spot on 0/30000\n\
acc/spot 2 on 50000/1400000 and (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>Each filter slot actually has a 'reject' slot and an 'accept'\n\
  slot. The reject slot is executed BEFORE the accept slot.\n\
</p>\n\
<p>It was mentioned earlier that after a reject test that doesn't\n\
  match, the default for following tests is 'accept', the reverse is\n\
  true for first, any non hf/cw spot is passed to the accept line,\n\
  which lets through everything else on HF.\n\
</p>\n\
<p>The last filter line in the example above lets through just\n\
  VHF/UHF spots from EU.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"HELP\">HELP</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"help_2\">help</span></h3>\n\
<ul>\n\
<li>help - The HELP Command\n\
</li>\n\
</ul>\n\
<p>HELP is available for a number of commands. The syntax is:-\n\
</p>\n\
<pre>HELP &lt;cmd&gt;\n\
</pre>\n\
<p>Where &lt;cmd&gt; is the name of the command you want help on.\n\
  All commands can be abbreviated, so SHOW/DX can be abbreviated to\n\
  SH/DX, ANNOUNCE can be shortened to AN and so on.\n\
</p>\n\
<p>Look at the APROPOS &lt;string&gt; command which will search the\n\
  help database for the &lt;string&gt; you specify and give you a\n\
  list of likely commands to look at with HELP.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"JOIN\">JOIN</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"join_.3Cgroup.3E\">join\n\
&lt;group&gt;</span></h3>\n\
<ul>\n\
<li>join &lt;group&gt; - Join a chat or conference group\n\
</li>\n\
</ul>\n\
<p>JOIN allows you to join a network wide conference group. To join\n\
  a group (called FOC in this case) type:-\n\
</p>\n\
<pre>JOIN FOC\n\
</pre>\n\
<p>See also CHAT, LEAVE, SHOW/CHAT\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"KILL\">KILL</span></h2>\n\
<h3><span class=\"mw-headline\"\n\
id=\"kill_.3Cfrom_msgno.3E-.3Cto_msgno.3E\">kill &lt;from\n\
msgno&gt;-&lt;to msgno&gt;</span></h3>\n\
<ul>\n\
<li>kill &lt;from msgno&gt;-&lt;to msgno&gt; - Delete a range of\n\
messages\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"kill_.3Cmsgno.3E_.5B.3Cmsgno...5D\">kill\n\
&lt;msgno&gt; [&lt;msgno..]</span></h3>\n\
<ul>\n\
<li>kill &lt;msgno&gt; [&lt;msgno..] - Delete a message from the\n\
local system\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\"\n\
id=\"kill_.3Cmsgno.3E_.5B.3Cmsgno.3E_....5D\">kill &lt;msgno&gt;\n\
[&lt;msgno&gt; ...]</span></h3>\n\
<ul>\n\
<li>kill &lt;msgno&gt; [&lt;msgno&gt; ...] - Remove or erase a\n\
message from the system\n\
</li>\n\
</ul>\n\
<p>You can get rid of any message to or originating from your\n\
  callsign using this command. You can remove more than one message\n\
  at a time.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"kill_from_.3Cregex.3E\">kill from\n\
&lt;regex&gt;</span></h3>\n\
<ul>\n\
<li>kill from &lt;regex&gt; - Delete messages FROM a callsign or\n\
pattern\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"kill_to_.3Cregex.3E\">kill to\n\
&lt;regex&gt;</span></h3>\n\
<ul>\n\
<li>kill to &lt;regex&gt; - Delete messages TO a callsign or\n\
pattern\n\
</li>\n\
</ul>\n\
<h2><span class=\"mw-headline\" id=\"LEAVE\">LEAVE</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"leave_.3Cgroup.3E\">leave\n\
&lt;group&gt;</span></h3>\n\
<ul>\n\
<li>leave &lt;group&gt; - Leave a chat or conference group\n\
</li>\n\
</ul>\n\
<p>LEAVE allows you to leave a network wide conference group. To\n\
  leave a group (called FOC in this case) type:-\n\
</p>\n\
<pre>LEAVE FOC\n\
</pre>\n\
<p>See also CHAT, JOIN, SHOW/CHAT\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"LINKS\">LINKS</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"links_2\">links</span></h3>\n\
<ul>\n\
<li>links - Show which nodes is physically connected\n\
</li>\n\
</ul>\n\
<p>This is a quick listing that shows which links are connected and\n\
  some information about them. See WHO for a list of all\n\
  connections.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"READ\">READ</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"read_2\">read</span></h3>\n\
<ul>\n\
<li>read - Read the next unread personal message addressed to you\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"read_.3Cmsgno.3E\">read\n\
&lt;msgno&gt;</span></h3>\n\
<ul>\n\
<li>read &lt;msgno&gt; - Read the specified message\n\
</li>\n\
</ul>\n\
<p>You can read any messages that are sent as 'non-personal' and\n\
  also any message either sent by or sent to your callsign.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"REJECT\">REJECT</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"reject_2\">reject</span></h3>\n\
<ul>\n\
<li>reject - Set a filter to reject something\n\
</li>\n\
</ul>\n\
<p>There are 2 types of filter, accept and reject. See HELP\n\
  FILTERING for more info.\n\
</p>\n\
<h3><span class=\"mw-headline\"\n\
id=\"reject.2Fannounce_.5B0-9.5D_.3Cpattern.3E\">reject/announce\n\
[0-9] &lt;pattern&gt;</span></h3>\n\
<ul>\n\
<li>reject/announce [0-9] &lt;pattern&gt; - Set a 'reject' filter\n\
line for announce\n\
</li>\n\
</ul>\n\
<p>A reject filter line means that if the announce matches this\n\
  filter it is passed onto the user. See HELP FILTERING for more\n\
  info. Please read this to understand how filters work - it will\n\
  save a lot of grief later on.\n\
</p>\n\
<p>You can use any of the following things in this line:-\n\
</p>\n\
<pre>info &lt;string&gt;            eg: iota or qsl\n\
by &lt;prefixes&gt;            eg: G,M,2\n\
origin &lt;prefixes&gt;\n\
origin_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
origin_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
origin_zone &lt;prefixes or numbers&gt;\n\
origin_state &lt;states&gt;                eg: VA,NH,RI,ME\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
by_state &lt;states&gt;                eg: VA,NH,RI,ME\n\
channel &lt;prefixes&gt;\n\
wx 1                     filter WX announces\n\
dest &lt;prefixes&gt;          eg: 6MUK,WDX      (distros)\n\
</pre>\n\
<p>some examples:-\n\
</p>\n\
<pre>rej/ann by_zone 14,15,16 and not by G,M,2\n\
</pre>\n\
<p>You can use the tag 'all' to reject everything eg:\n\
</p>\n\
<pre>rej/ann all\n\
</pre>\n\
<p>but this probably for advanced users...\n\
</p>\n\
<h3><span class=\"mw-headline\"\n\
id=\"reject.2Fspots_.5B0-9.5D_.3Cpattern.3E\">reject/spots [0-9]\n\
&lt;pattern&gt;</span></h3>\n\
<ul>\n\
<li>reject/spots [0-9] &lt;pattern&gt; - Set a 'reject' filter\n\
line for spots\n\
</li>\n\
</ul>\n\
<p>A reject filter line means that if the spot matches this filter\n\
  it is dumped (not passed on). See HELP FILTERING for more info.\n\
  Please read this to understand how filters work - it will save a\n\
  lot of grief later on.\n\
</p>\n\
<p>You can use any of the following things in this line:-\n\
</p>\n\
<pre>freq &lt;range&gt;           eg: 0/30000 or hf or hf/cw or 6m,4m,2m\n\
on &lt;range&gt;             same as 'freq'\n\
call &lt;prefixes&gt;        eg: G,PA,HB9\n\
info &lt;string&gt;          eg: iota or qsl\n\
by &lt;prefixes&gt;\n\
call_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
call_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
call_zone &lt;prefixes or numbers&gt;\n\
call_state &lt;states&gt;                eg: VA,NH,RI,ME\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
by_state &lt;states&gt;                eg: VA,NH,RI,ME\n\
origin &lt;prefixes&gt;\n\
channel &lt;prefixes&gt;\n\
</pre>\n\
<p>For frequencies, you can use any of the band names defined in\n\
  SHOW/BANDS and you can use a subband name like: cw, rtty, data,\n\
  ssb - thus: hf/ssb. You can also just have a simple range like:\n\
  0/30000 - this is more efficient than saying simply: on HF (but\n\
  don't get too hung up about that)\n\
</p>\n\
<p>some examples:-\n\
</p>\n\
<pre>rej/spot 1 on hf\n\
rej/spot 2 on vhf and not (by_zone 14,15,16 or call_zone 14,15,16)\n\
</pre>\n\
<p>You can use the tag 'all' to reject everything eg:\n\
</p>\n\
<pre>rej/spot 3 all\n\
</pre>\n\
<p>but this probably for advanced users...\n\
</p>\n\
<h3><span class=\"mw-headline\"\n\
id=\"reject.2Fwcy_.5B0-9.5D_.3Cpattern.3E\">reject/wcy [0-9]\n\
&lt;pattern&gt;</span></h3>\n\
<ul>\n\
<li>reject/wcy [0-9] &lt;pattern&gt; - set a 'reject' WCY filter\n\
</li>\n\
</ul>\n\
<p>It is unlikely that you will want to do this, but if you do then\n\
  you can filter on the following fields:-\n\
</p>\n\
<pre>by &lt;prefixes&gt;            eg: G,M,2\n\
origin &lt;prefixes&gt;\n\
origin_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
origin_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
origin_zone &lt;prefixes or numbers&gt;\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
channel &lt;prefixes&gt;\n\
</pre>\n\
<p>There are no examples because WCY Broadcasts only come from one\n\
  place and you either want them or not (see UNSET/WCY if you don't\n\
  want them).\n\
</p>\n\
<p>This command is really provided for future use.\n\
</p>\n\
<p>See HELP FILTER for information.\n\
</p>\n\
<h3><span class=\"mw-headline\"\n\
id=\"reject.2Fwwv_.5B0-9.5D_.3Cpattern.3E\">reject/wwv [0-9]\n\
&lt;pattern&gt;</span></h3>\n\
<ul>\n\
<li>reject/wwv [0-9] &lt;pattern&gt; - set a 'reject' WWV filter\n\
</li>\n\
</ul>\n\
<p>It is unlikely that you will want to do this, but if you do then\n\
  you can filter on the following fields:-\n\
</p>\n\
<pre>by &lt;prefixes&gt;            eg: G,M,2\n\
origin &lt;prefixes&gt;\n\
origin_dxcc &lt;prefixes or numbers&gt;    eg: 61,62 (from eg: sh/pre G)\n\
origin_itu &lt;prefixes or numbers&gt;     or: G,GM,GW\n\
origin_zone &lt;prefixes or numbers&gt;\n\
by_dxcc &lt;prefixes or numbers&gt;\n\
by_itu &lt;prefixes or numbers&gt;\n\
by_zone &lt;prefixes or numbers&gt;\n\
channel &lt;prefixes&gt;\n\
</pre>\n\
<p>for example\n\
</p>\n\
<pre>reject/wwv by_zone 14,15,16\n\
</pre>\n\
<p>is probably the only useful thing to do (which will only show WWV\n\
  broadcasts by stations in the US).\n\
</p>\n\
<p>See HELP FILTER for information.\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"REPLY\">REPLY</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"reply_2\">reply</span></h3>\n\
<ul>\n\
<li>reply = Reply (privately) to the last message that you have\n\
read\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"reply_.3Cmsgno.3E\">reply\n\
&lt;msgno&gt;</span></h3>\n\
<ul>\n\
<li>reply &lt;msgno&gt; - Reply (privately) to the specified\n\
message\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"reply_b_.3Cmsgno.3E\">reply b\n\
&lt;msgno&gt;</span></h3>\n\
<ul>\n\
<li>reply b &lt;msgno&gt; - Reply as a Bulletin to the specified\n\
message\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"reply_noprivate_.3Cmsgno.3E\">reply\n\
noprivate &lt;msgno&gt;</span></h3>\n\
<ul>\n\
<li>reply noprivate &lt;msgno&gt; - Reply as a Bulletin to the\n\
specified message\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"reply_rr_.3Cmsgno.3E\">reply rr\n\
&lt;msgno&gt;</span></h3>\n\
<ul>\n\
<li>reply rr &lt;msgno&gt; - Reply to the specified message with\n\
read receipt\n\
</li>\n\
</ul>\n\
<p>You can reply to a message and the subject will automatically\n\
  have \"Re:\" inserted in front of it, if it isn't already present.\n\
  You can also use all the extra qualifiers such as RR, PRIVATE,\n\
  NOPRIVATE, B that you can use with the SEND command (see SEND for\n\
  further details)\n\
</p>\n\
<h2><span class=\"mw-headline\" id=\"SEND\">SEND</span></h2>\n\
<h3><span class=\"mw-headline\"\n\
id=\"send_.3Ccall.3E_.5B.3Ccall.3E_....5D\">send &lt;call&gt;\n\
[&lt;call&gt; ...]</span></h3>\n\
<ul>\n\
<li>send &lt;call&gt; [&lt;call&gt; ...] - Send a message to one\n\
or more callsigns\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"send_copy_.3Cmsgno.3E_.3Ccall.3E\">send\n\
copy &lt;msgno&gt; &lt;call&gt;</span></h3>\n\
<ul>\n\
<li>send copy &lt;msgno&gt; &lt;call&gt; - Send a copy of a\n\
message to someone\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"send_noprivate_.3Ccall.3E\">send\n\
noprivate &lt;call&gt;</span></h3>\n\
<ul>\n\
<li>send noprivate &lt;call&gt; - Send a message to all stations\n\
</li>\n\
</ul>\n\
<p>All the SEND commands will create a message which will be sent\n\
  either to an individual callsign or to one of the 'bulletin'\n\
  addresses.\n\
  SEND &lt;call&gt; on its own acts as though you had typed SEND\n\
  PRIVATE, that is it will mark the message as personal and send it\n\
  to the cluster node that that callsign is connected to. If the\n\
  &lt;call&gt; you have specified is in fact a known bulletin\n\
  category on your node (eg: ALL) then the message should\n\
  automatically become a bulletin. You can have more than one\n\
  callsign in all of the SEND commands.\n\
</p>\n\
<p>You can have multiple qualifiers so that you can have for\n\
  example:-\n\
</p>\n\
<pre>SEND RR COPY 123 PRIVATE G1TLH G0RDI\n\
</pre>\n\
<p>which should send a copy of message 123 to G1TLH and G0RDI and\n\
  you will receive a read receipt when they have read the message.\n\
</p>\n\
<p>SB is an alias for SEND NOPRIVATE (or send a bulletin in BBS\n\
  speak) SP is an alias for SEND PRIVATE\n\
</p>\n\
<p>The system will ask you for a subject. Conventionally this should\n\
  be no longer than 29 characters for compatibility. Most modern\n\
  cluster software should accept more.\n\
</p>\n\
<p>You will now be prompted to start entering your text.\n\
</p>\n\
<p>You finish the message by entering '/EX' on a new line. For\n\
  instance:\n\
</p>\n\
<pre>...\n\
bye then Jim\n\
73 Dirk\n\
/ex\n\
</pre>\n\
<p>If you have started a message and you don't want to keep it then\n\
  you can abandon the message with '/ABORT' on a new line, like:-\n\
</p>\n\
<pre>line 1\n\
line 2\n\
oh I just can't be bothered with this\n\
/abort\n\
</pre>\n\
<p>If you abort the message it will NOT be sent.\n\
</p>\n\
<p>When you are entering the text of your message, most normal\n\
  output (such as DX announcements and so on are suppressed and\n\
  stored for latter display (upto 20 such lines are stored, as new\n\
  ones come along, so the oldest lines are dropped).\n\
</p>\n\
<p>Also, you can enter normal commands commands (and get the output\n\
  immediately) whilst in the middle of a message. You do this by\n\
  typing the command preceeded by a '/' character on a new line,\n\
  so:-\n\
</p>\n\
<pre>/dx g1tlh 144010 strong signal\n\
</pre>\n\
<p>Will issue a dx annoucement to the rest of the cluster.\n\
</p>\n\
<p>Also, you can add the output of a command to your message by\n\
  preceeding the command with '//', thus&nbsp;:-\n\
</p>\n\
<pre>//sh/vhftable\n\
</pre>\n\
<p>This will show YOU the output from SH/VHFTABLE and also store it\n\
  in the message.\n\
</p>\n\
<p>You can carry on with the message until you are ready to send it.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"send_private_.3Ccall.3E\">send\n\
private &lt;call&gt;</span></h3>\n\
<ul>\n\
<li>send private &lt;call&gt; - Send a personal message\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"send_rr_.3Ccall.3E\">send rr\n\
&lt;call&gt;</span></h3>\n\
<ul>\n\
<li>send rr &lt;call&gt; - Send a message and ask for a read\n\
receipt\n\
</li>\n\
</ul>\n\
<h2><span class=\"mw-headline\" id=\"SET\">SET</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"set.2Faddress_.3Cyour_address.3E\">set/address\n\
&lt;your address&gt;</span></h3>\n\
<ul>\n\
<li>set/address &lt;your address&gt; - Record your postal address\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fannounce\">set/announce</span></h3>\n\
<ul>\n\
<li>set/announce - Allow announce messages to come out on your\n\
terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fanntalk\">set/anntalk</span></h3>\n\
<ul>\n\
<li>set/anntalk - Allow talk like announce messages on your\n\
terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fbeep\">set/beep</span></h3>\n\
<ul>\n\
<li>set/beep - Add a beep to DX and other messages on your\n\
terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fdx\">set/dx</span></h3>\n\
<ul>\n\
<li>set/dx - Allow DX messages to come out on your terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fdxcq\">set/dxcq</span></h3>\n\
<ul>\n\
<li>set/dxcq - Show CQ Zones on the end of DX announcements\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fdxgrid\">set/dxgrid</span></h3>\n\
<ul>\n\
<li>set/dxgrid - Allow QRA Grid Squares on the end of DX\n\
announcements\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fdxitu\">set/dxitu</span></h3>\n\
<ul>\n\
<li>set/dxitu - Show ITU Zones on the end of DX announcements\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fecho\">set/echo</span></h3>\n\
<ul>\n\
<li>set/echo - Make the cluster echo your input\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Femail_.3Cemail.3E\">set/email\n\
&lt;email&gt;</span></h3>\n\
<ul>\n\
<li>set/email &lt;email&gt; - Set email address(es) and forward\n\
your personals\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fhere\">set/here</span></h3>\n\
<ul>\n\
<li>set/here - Tell the system you are present at your terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fhomenode_.3Cnode.3E\">set/homenode\n\
&lt;node&gt;</span></h3>\n\
<ul>\n\
<li>set/homenode &lt;node&gt; - Set your normal cluster callsign\n\
</li>\n\
</ul>\n\
<p>Tell the cluster system where you normally connect to. Any\n\
  Messages sent to you will normally find their way there should you\n\
  not be connected. eg:-\n\
</p>\n\
<pre>SET/HOMENODE gb7djk\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"set.2Flanguage_.3Clang.3E\">set/language\n\
&lt;lang&gt;</span></h3>\n\
<ul>\n\
<li>set/language &lt;lang&gt; - Set the language you want to use\n\
</li>\n\
</ul>\n\
<p>You can select the language that you want the cluster to use.\n\
  Currently the languages available are en (English), de (German),\n\
  es (Spanish), Czech (cz), French (fr), Portuguese (pt), Italian\n\
  (it) and nl (Dutch).\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"set.2Flocation_.3Clat_.26_long.3E\">set/location\n\
&lt;lat &amp; long&gt;</span></h3>\n\
<ul>\n\
<li>set/location &lt;lat &amp; long&gt; - Set your latitude and\n\
longitude\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Flogininfo\">set/logininfo</span></h3>\n\
<ul>\n\
<li>set/logininfo - Inform when a station logs in/out locally\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fname_.3Cyour_name.3E\">set/name\n\
&lt;your name&gt;</span></h3>\n\
<ul>\n\
<li>set/name &lt;your name&gt; - Set your name\n\
</li>\n\
</ul>\n\
<p>Tell the system what your name is eg:-\n\
</p>\n\
<pre>SET/NAME Dirk\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fpage_.3Clines_per_page.3E\">set/page\n\
&lt;lines per page&gt;</span></h3>\n\
<ul>\n\
<li>set/page &lt;lines per page&gt; - Set the lines per page\n\
</li>\n\
</ul>\n\
<p>Tell the system how many lines you wish on a page when the number\n\
  of line of output from a command is more than this. The default is\n\
  20. Setting it explicitly to 0 will disable paging.\n\
</p>\n\
<pre>SET/PAGE 30\n\
SET/PAGE 0\n\
</pre>\n\
<p>The setting is stored in your user profile.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fpassword\">set/password</span></h3>\n\
<ul>\n\
<li>set/password - Set your own password\n\
</li>\n\
</ul>\n\
<p>This command only works for a 'telnet' user (currently). It will\n\
  only work if you have a password already set. This initial\n\
  password can only be set by the sysop.\n\
</p>\n\
<p>When you execute this command it will ask you for your old\n\
  password, then ask you to type in your new password twice (to make\n\
  sure you get it right). You may or may not see the data echoed on\n\
  the screen as you type, depending on the type of telnet client you\n\
  have.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fprompt_.3Cstring.3E\">set/prompt\n\
&lt;string&gt;</span></h3>\n\
<ul>\n\
<li>set/prompt &lt;string&gt; - Set your prompt to &lt;string&gt;\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fqra_.3Clocator.3E\">set/qra\n\
&lt;locator&gt;</span></h3>\n\
<ul>\n\
<li>set/qra &lt;locator&gt; - Set your QRA Grid locator\n\
</li>\n\
</ul>\n\
<p>Tell the system what your QRA (or Maidenhead) locator is. If you\n\
  have not done a SET/LOCATION then your latitude and longitude will\n\
  be set roughly correctly (assuming your locator is\n\
  correct&nbsp;;-). For example:-\n\
</p>\n\
<pre>SET/QRA JO02LQ\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fqth_.3Cyour_qth.3E\">set/qth\n\
&lt;your qth&gt;</span></h3>\n\
<ul>\n\
<li>set/qth &lt;your qth&gt; - Set your QTH\n\
</li>\n\
</ul>\n\
<p>Tell the system where you are. For example:-\n\
</p>\n\
<pre>SET/QTH East Dereham, Norfolk\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fstartup\">set/startup</span></h3>\n\
<ul>\n\
<li>set/startup - Create your own startup script\n\
</li>\n\
</ul>\n\
<p>Create a startup script of DXSpider commands which will be\n\
  executed everytime that you login into this node. You can only\n\
  input the whole script afresh, it is not possible to 'edit' it.\n\
  Inputting a new script is just like typing in a message using\n\
  SEND. To finish inputting type: /EX on a newline, to abandon the\n\
  script type: /ABORT.\n\
</p>\n\
<p>You may find the (curiously named) command BLANK useful to break\n\
  up the output. If you simply want a blank line, it is easier to\n\
  input one or more spaces and press the &lt;return&gt; key.\n\
</p>\n\
<p>See UNSET/STARTUP to remove a script.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"set.2Ftalk\">set/talk</span></h3>\n\
<ul>\n\
<li>set/talk - Allow TALK messages to come out on your terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fusstate\">set/usstate</span></h3>\n\
<ul>\n\
<li>set/usstate - Allow US State info on the end of DX\n\
announcements\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fwcy\">set/wcy</span></h3>\n\
<ul>\n\
<li>set/wcy - Allow WCY messages to come out on your terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fwwv\">set/wwv</span></h3>\n\
<ul>\n\
<li>set/wwv - Allow WWV messages to come out on your terminal\n\
</li>\n\
</ul>\n\
<h3><span class=\"mw-headline\" id=\"set.2Fwx\">set/wx</span></h3>\n\
<ul>\n\
<li>set/wx - Allow WX messages to come out on your terminal\n\
</li>\n\
</ul>\n\
<h2><span class=\"mw-headline\" id=\"SHOW\">SHOW</span></h2>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fchat\">show/chat</span></h3>\n\
<ul>\n\
<li>show/chat [&lt;group&gt;] [&lt;lines&gt;]\n\
</li>\n\
</ul>\n\
<p>This command allows you to see any chat or conferencing that has\n\
  occurred whilst you were away. SHOW/CHAT on its own will show data\n\
  for all groups. If you use a group name then it will show only\n\
  chat for that group.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fconfiguration\">show/configuration</span></h3>\n\
<ul>\n\
<li>show/configuration [&lt;node&gt;]\n\
</li>\n\
</ul>\n\
<p>This command allows you to see all the users that can be seen and\n\
  the nodes to which they are connected.\n\
</p>\n\
<p>This command is normally abbreviated to: sh/c\n\
</p>\n\
<p>Normally, the list returned will be just for the nodes from your\n\
  country (because the list otherwise will be very long).\n\
</p>\n\
<pre>SH/C ALL\n\
</pre>\n\
<p>will produce a complete list of all nodes.\n\
</p>\n\
<p>BE WARNED: the list that is returned can be VERY long\n\
</p>\n\
<p>It is possible to supply a node or part of a prefix and you will\n\
  get a list of the users for that node or list of nodes starting\n\
  with that prefix.\n\
</p>\n\
<pre>SH/C GB7DJK\n\
SH/C SK\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fconfiguration.2Fnode\">show/configuration/node</span></h3>\n\
<ul>\n\
<li>show/configuration/node\n\
</li>\n\
</ul>\n\
<p>Show all the nodes connected to this node.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fcontest\">show/contest</span></h3>\n\
<ul>\n\
<li>show/contest &lt;year and month&gt;\n\
</li>\n\
</ul>\n\
<p>Show all known contests which are maintained at <a\n\
rel=\"nofollow\" class=\"external free\"\n\
href=\"http://www.sk3bg.se/contest/\">http://www.sk3bg.se/contest/</a>\n\
  for a particular month or year. The format is reasonably flexible.\n\
  For example:-\n\
</p>\n\
<pre>SH/CONTEST sep2003\n\
SH/CONTEST 03 march\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fdate\">show/date</span></h3>\n\
<ul>\n\
<li>show/date [&lt;prefix&gt;|&lt;callsign&gt;]\n\
</li>\n\
</ul>\n\
<p>This is very nearly the same as SHOW/TIME, the only difference\n\
  the format of the date string if no arguments are given.\n\
</p>\n\
<p>If no prefixes or callsigns are given then this command returns\n\
  the local time and UTC as the computer has it right now. If you\n\
  give some prefixes then it will show UTC and UTC + the local\n\
  offset (not including DST) at the prefixes or callsigns that you\n\
  specify.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fdb0sdx\">show/db0sdx</span></h3>\n\
<ul>\n\
<li>show/db0sdx &lt;callsign&gt;\n\
</li>\n\
</ul>\n\
<p>This command queries the DB0SDX QSL server on the internet and\n\
  returns any information available for that callsign. This service\n\
  is provided for users of this software by <a rel=\"nofollow\"\n\
class=\"external free\" href=\"http://www.qslinfo.de\">http://www.qslinfo.de</a>.\n\
</p>\n\
<p>See also SHOW/QRZ, SHOW/WM7D.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fdx\">show/dx</span></h3>\n\
<ul>\n\
<li>show/dx\n\
</li>\n\
</ul>\n\
<p>If you just type SHOW/DX you will get the last so many spots\n\
  (sysop configurable, but usually 10).\n\
</p>\n\
<p>In addition you can add any number of these commands in very\n\
  nearly any order to the basic SHOW/DX command, they are:-\n\
</p>\n\
<pre>on &lt;band&gt;       - eg 160m 20m 2m 23cm 6mm\n\
on &lt;region&gt;     - eg hf vhf uhf shf      (see SHOW/BANDS)\n\
on &lt;from&gt;/&lt;to&gt;  - eg 1000/4000 14000-30000  (in Khz)\n\
   &lt;from&gt;-&lt;to&gt;\n\
&lt;number&gt;        - the number of spots you want\n\
&lt;from&gt;-&lt;to&gt;     - &lt;from&gt; spot no &lt;to&gt; spot no in the selected list\n\
&lt;from&gt;/&lt;to&gt;\n\
&lt;prefix&gt;        - for a spotted callsign beginning with &lt;prefix&gt;\n\
*&lt;suffix&gt;       - for a spotted callsign ending in &lt;suffix&gt;\n\
*&lt;string&gt;*      - for a spotted callsign containing &lt;string&gt;\n\
day &lt;number&gt;    - starting &lt;number&gt; days ago\n\
day &lt;from&gt;-&lt;to&gt; - &lt;from&gt; days &lt;to&gt; days ago\n\
&lt;from&gt;/&lt;to&gt;\n\
info &lt;text&gt;     - any spots containing &lt;text&gt; in the info or remarks\n\
by &lt;call&gt;       - any spots spotted by &lt;call&gt; (spotter &lt;call&gt; is the same).\n\
qsl             - this automatically looks for any qsl info on the call\n\
  held in the spot database.\n\
iota [&lt;iota&gt;]   - If the iota island number is missing it will look for\n\
  the string iota and anything which looks like an iota\n\
  island number. If you specify then it will look for\n\
  that island.\n\
qra [&lt;locator&gt;] - this will look for the specific locator if you specify\n\
  one or else anything that looks like a locator.\n\
dxcc            - treat the prefix as a 'country' and look for spots\n\
  from that country regardless of actual prefix. eg dxcc oq2\n\
  You can also use this with the 'by' keyword. eg by W dxcc\n\
real or rt      - Format the output the same as for real time spots. The\n\
  formats are deliberately different (so you can tell\n\
  one sort from the other). This is useful for some\n\
  logging programs that can't cope with normal sh/dx\n\
  output. An alias of SHOW/FDX is available.\n\
filter          - Filter the spots, before output, with the user's\n\
  spot filter. An alias of SHOW/MYDX is available.\n\
zone &lt;zones&gt;    - look for spots in the cq zone (or zones) specified.\n\
  zones are numbers separated by commas.\n\
by_zone &lt;zones&gt; - look for spots spotted by people in the cq zone\n\
  specified.\n\
itu &lt;itus&gt;      - look for spots in the itu zone (or zones) specified\n\
  itu zones are numbers separated by commas.\n\
by_itu &lt;itus&gt;   - look for spots spotted by people in the itu zone\n\
  specified.\n\
state &lt;list&gt;    - look for spots in the US state (or states) specified\n\
  The list is two letter state codes separated by commas.\n\
by_state &lt;list&gt; - look for spots spotted by people in the US state\n\
  specified.\n\
</pre>\n\
<p>Examples...\n\
</p>\n\
<pre>SH/DX 9m0\n\
SH/DX on 20m info iota\n\
SH/DX 9a on vhf day 30\n\
SH/DX rf1p qsl\n\
SH/DX iota\n\
SH/DX iota eu-064\n\
SH/DX qra jn86\n\
SH/DX dxcc oq2\n\
SH/DX dxcc oq2 by w dxcc\n\
SH/DX zone 4,5,6\n\
SH/DX by_zone 4,5,6\n\
SH/DX state in,oh\n\
SH/DX by_state in,oh\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fdxcc\">show/dxcc</span></h3>\n\
<ul>\n\
<li>show/dxcc &lt;prefix&gt;\n\
</li>\n\
</ul>\n\
<p>This command takes the &lt;prefix&gt; (which can be a full or\n\
  partial callsign if desired), looks up which internal country\n\
  number it is and then displays all the spots as per SH/DX for that\n\
  country.\n\
</p>\n\
<p>This is now an alias for 'SHOW/DX DXCC'\n\
</p>\n\
<p>The options for SHOW/DX also apply to this command. e.g.\n\
</p>\n\
<pre>SH/DXCC G\n\
SH/DXCC W on 20m iota\n\
</pre>\n\
<p>This can be done with the SHOW/DX command like this:-\n\
</p>\n\
<pre>SH/DX dxcc g\n\
SH/DX dxcc w on 20m iota\n\
</pre>\n\
<p>This is an alias for: SH/DX dxcc\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fdxqsl\">show/dxqsl</span></h3>\n\
<ul>\n\
<li>show/dxqsl &lt;callsign&gt;\n\
</li>\n\
</ul>\n\
<p>The node collects information from the comment fields in spots\n\
  (things like 'VIA EA7WA' or 'QSL-G1TLH') and stores these in a\n\
  database.\n\
</p>\n\
<p>This command allows you to interrogate that database and if the\n\
  callsign is found will display the manager(s) that people have\n\
  spotted. This information is NOT reliable, but it is normally\n\
  reasonably accurate if it is spotted enough times.\n\
</p>\n\
<p>For example:-\n\
</p>\n\
<pre>sh/dxqsl 4k9w\n\
</pre>\n\
<p>You can check the raw input spots yourself with:-\n\
</p>\n\
<pre>sh/dx 4k9w qsl\n\
</pre>\n\
<p>This gives you more background information.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fdxstats\">show/dxstats</span></h3>\n\
<ul>\n\
<li>show/dxstats [days] [date]�[0m\n\
</li>\n\
</ul>\n\
<p>Show the total DX spots for the last &lt;days&gt; no of days\n\
  (default is 31), starting from a &lt;date&gt; (default: today).\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Ffdx\">show/fdx</span></h3>\n\
<ul>\n\
<li>show/fdx\n\
</li>\n\
</ul>\n\
<p>Normally SHOW/DX outputs spot data in a different format to the\n\
  realtime data. This is a deliberate policy (so you can tell the\n\
  difference between the two). Some logging programs cannot handle\n\
  this so SHOW/FDX outputs historical data in real time format.\n\
</p>\n\
<p>This is an alias for: SHOW/DX real\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Ffiles\">show/files</span></h3>\n\
<ul>\n\
<li>show/files [&lt;filearea&gt; [&lt;string&gt;]]\n\
</li>\n\
</ul>\n\
<p>SHOW/FILES on its own will show you a list of the various\n\
  fileareas available on the system. To see the contents of a\n\
  particular file area type:-\n\
</p>\n\
<pre>SH/FILES &lt;filearea&gt;\n\
</pre>\n\
<p>where &lt;filearea&gt; is the name of the filearea you want to\n\
  see the contents of.\n\
</p>\n\
<p>You can also use shell globbing characters like '*' and '?' in a\n\
  string to see a selection of files in a filearea eg:-\n\
</p>\n\
<pre>SH/FILES bulletins arld*\n\
</pre>\n\
<p>See also TYPE - to see the contents of a file.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Ffilter\">show/filter</span></h3>\n\
<ul>\n\
<li>show/filter\n\
</li>\n\
</ul>\n\
<p>Show the contents of all the filters that are set. This command\n\
  displays all the filters set - for all the various categories.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fhfstats\">show/hfstats</span></h3>\n\
<ul>\n\
<li>show/hfstats [days] [date]\n\
</li>\n\
</ul>\n\
<p>Show the HF DX spots breakdown by band for the last &lt;days&gt;\n\
  no of days (default is 31), starting from a &lt;date&gt; (default:\n\
  today).\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fhftable\">show/hftable</span></h3>\n\
<ul>\n\
<li>show/hftable [days] [date] [prefix ...]\n\
</li>\n\
</ul>\n\
<p>Show the HF DX Spotter table for the list of prefixes for the\n\
  last &lt;days&gt; no of days (default is 31), starting from a\n\
  &lt;date&gt; (default: today).\n\
</p>\n\
<p>If there are no prefixes then it will show the table for your\n\
  country.\n\
</p>\n\
<p>Remember that some countries have more than one \"DXCC country\" in\n\
  them (eg G&nbsp;:-), to show them (assuming you are not in G\n\
  already which is specially treated in the code) you must list all\n\
  the relevant prefixes\n\
</p>\n\
<pre>sh/hftable g gm gd gi gj gw gu\n\
</pre>\n\
<p>Note that the prefixes are converted into country codes so you\n\
  don't have to list all possible prefixes for each country.\n\
</p>\n\
<p>If you want more or less days than the default simply include the\n\
  number you require:-\n\
</p>\n\
<pre>sh/hftable 20 pa\n\
</pre>\n\
<p>If you want to start at a different day, simply add the date in\n\
  some recognizable form:-\n\
</p>\n\
<pre>sh/hftable 2 25nov02\n\
sh/hftable 2 25-nov-02\n\
sh/hftable 2 021125\n\
sh/hftable 2 25/11/02\n\
</pre>\n\
<p>This will show the stats for your DXCC for that CQWW contest\n\
  weekend.\n\
</p>\n\
<p>You can specify either prefixes or full callsigns (so you can see\n\
  how you did against all your mates). You can also say 'all' which\n\
  will then print the worldwide statistics.\n\
</p>\n\
<pre>sh/hftable all\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fmoon\">show/moon</span></h3>\n\
<ul>\n\
<li>show/moon [ndays] [&lt;prefix&gt;|&lt;callsign&gt;]�[0m\n\
</li>\n\
</ul>\n\
<p>Show the Moon rise and set times for a (list of) prefixes or\n\
  callsigns, together with the azimuth and elevation of the sun\n\
  currently at those locations.\n\
</p>\n\
<p>If you don't specify any prefixes or callsigns, it will show the\n\
  times for your QTH (assuming you have set it with either\n\
  SET/LOCATION or SET/QRA), together with the current azimuth and\n\
  elevation.\n\
</p>\n\
<p>In addition, it will show the illuminated fraction of the moons\n\
  disk.\n\
</p>\n\
<p>If all else fails it will show the Moonrise and set times for the\n\
  node that you are connected to.\n\
</p>\n\
<p>For example:-\n\
</p>\n\
<pre>SH/MOON\n\
SH/MOON G1TLH W5UN\n\
</pre>\n\
<p>You can also use this command to see into the past or the future,\n\
  so if you want to see yesterday's times then do:-\n\
</p>\n\
<pre>SH/MOON -1\n\
</pre>\n\
<p>or in three days time:-\n\
</p>\n\
<pre>SH/MOON +3 W9\n\
</pre>\n\
<p>Upto 366 days can be checked both in the past and in the future.\n\
</p>\n\
<p>Please note that the rise and set times are given as the UT times\n\
  of rise and set on the requested UT day.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fmuf\">show/muf</span></h3>\n\
<ul>\n\
<li>show/muf &lt;prefix&gt; [&lt;hours&gt;][long]\n\
</li>\n\
</ul>\n\
<p>This command allow you to estimate the likelihood of you\n\
  contacting a station with the prefix you have specified. The\n\
  output assumes a modest power of 20dBW and receiver sensitivity of\n\
  -123dBm (about 0.15muV/10dB SINAD)\n\
</p>\n\
<p>The result predicts the most likely operating frequencies and\n\
  signal levels for high frequency (shortwave) radio propagation\n\
  paths on specified days of the year and hours of the day. It is\n\
  most useful for paths between 250 km and 6000 km, but can be used\n\
  with reduced accuracy for paths shorter or longer than this.\n\
</p>\n\
<p>The command uses a routine MINIMUF 3.5 developed by the U.S. Navy\n\
  and used to predict the MUF given the predicted flux, day of the\n\
  year, hour of the day and geographic coordinates of the\n\
  transmitter and receiver. This routine is reasonably accurate for\n\
  the purposes here, with a claimed RMS error of 3.8 MHz, but much\n\
  smaller and less complex than the programs used by major shortwave\n\
  broadcasting organizations, such as the Voice of America.\n\
</p>\n\
<p>The command will display some header information detailing its\n\
  assumptions, together with the locations, latitude and longitudes\n\
  and bearings. It will then show UTC (UT), local time at the other\n\
  end (LT), calculate the MUFs, Sun zenith angle at the midpoint of\n\
  the path (Zen) and the likely signal strengths. Then for each\n\
  frequency for which the system thinks there is a likelihood of a\n\
  circuit it prints a value.\n\
</p>\n\
<p>The value is currently a likely S meter reading based on the\n\
  conventional 6dB / S point scale. If the value has a '+' appended\n\
  it means that it is 1/2 an S point stronger. If the value is\n\
  preceded by an 'm' it means that there is likely to be much fading\n\
  and by an 's' that the signal is likely to be noisy.\n\
</p>\n\
<p>By default SHOW/MUF will show the next two hours worth of data.\n\
  You can specify anything up to 24 hours worth of data by appending\n\
  the no of hours required after the prefix. For example:-\n\
</p>\n\
<pre>SH/MUF W\n\
</pre>\n\
<p>produces:\n\
</p>\n\
<pre>RxSens: -123 dBM SFI: 159   R: 193   Month: 10   Day: 21\n\
Power&nbsp;:   20 dBW    Distance:  6283 km    Delay: 22.4 ms\n\
Location                       Lat / Long           Azim\n\
East Dereham, Norfolk          52 41 N 0 57 E         47\n\
United-States-W                43 0 N 87 54 W        299\n\
UT LT  MUF Zen  1.8  3.5  7.0 10.1 14.0 18.1 21.0 24.9 28.0 50.0\n\
18 23 11.5 -35  mS0+ mS2   S3\n\
19  0 11.2 -41  mS0+ mS2   S3\n\
</pre>\n\
<p>indicating that you will have weak, fading circuits on top band\n\
  and 80m but usable signals on 40m (about S3).\n\
</p>\n\
<p>inputing:-\n\
</p>\n\
<pre>SH/MUF W 24\n\
</pre>\n\
<p>will get you the above display, but with the next 24 hours worth\n\
  of propagation data.\n\
</p>\n\
<pre>SH/MUF W L 24\n\
SH/MUF W 24 Long\n\
</pre>\n\
<p>Gives you an estimate of the long path propagation characterics.\n\
  It should be noted that the figures will probably not be very\n\
  useful, nor terrible accurate, but it is included for\n\
  completeness.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fmydx\">show/mydx</span></h3>\n\
<ul>\n\
<li>show/mydx\n\
</li>\n\
</ul>\n\
<p>SHOW/DX potentially shows all the spots available in the system.\n\
  Using SHOW/MYDX will, instead, filter the availble spots using any\n\
  spot filter that you have set, first.\n\
</p>\n\
<p>This command, together with ACCEPT/SPOT or REJECT/SPOT, will\n\
  allow you to customise the spots that you receive.\n\
</p>\n\
<p>So if you have said: ACC/SPOT on hf, doing a SHOW/MYDX will now\n\
  only, ever, show HF spots. </p>\n\
<p>All the other options on SH/DX can still be used.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fnewconfiguration\">show/newconfiguration</span></h3>\n\
<ul>\n\
<li>show/newconfiguration [&lt;node&gt;]\n\
</li>\n\
</ul>\n\
<p>This command allows you to see all the users that can be seen and\n\
  the nodes to which they are connected.\n\
</p>\n\
<p>This command produces essentially the same information as\n\
  SHOW/CONFIGURATION except that it shows all the duplication of any\n\
  routes that might be present It also uses a different format which\n\
  may not take up quite as much space if you don't have any loops.\n\
</p>\n\
<p>BE WARNED: the list that is returned can be VERY long\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fnewconfiguration.2Fnode\">show/newconfiguration/node</span></h3>\n\
<ul>\n\
<li>show/newconfiguration/node\n\
</li>\n\
</ul>\n\
<p>Show all the nodes connected to this node in the new format.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fprefix\">show/prefix</span></h3>\n\
<ul>\n\
<li>show/prefix &lt;callsign&gt;\n\
</li>\n\
</ul>\n\
<p>This command takes the &lt;callsign&gt; (which can be a full or\n\
  partial callsign or a prefix), looks up which internal country\n\
  number it is and then displays all the relevant prefixes for that\n\
  country together with the internal country no, the CQ and ITU\n\
  regions.\n\
</p>\n\
<p>See also SHOW/DXCC\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fqra\">show/qra</span></h3>\n\
<ul>\n\
<li>show/qra &lt;lat&gt; &lt;long&gt;\n\
</li>\n\
</ul>\n\
<p>This is a multipurpose command that allows you either to\n\
  calculate the distance and bearing between two locators or (if\n\
  only one locator is given on the command line) the distance and\n\
  beraing from your station to the locator. For example:-\n\
</p>\n\
<pre>SH/QRA IO92QL\n\
SH/QRA JN06 IN73\n\
</pre>\n\
<p>The first example will show the distance and bearing to the\n\
  locator from yourself, the second example will calculate the\n\
  distance and bearing from the first locator to the second. You can\n\
  use 4 or 6 character locators.\n\
</p>\n\
<p>It is also possible to convert a latitude and longitude to a\n\
  locator by using this command with a latitude and longitude as an\n\
  argument, for example:-\n\
</p>\n\
<pre>SH/QRA 52 41 N 0 58 E\n\
</pre>\n\
<ul>\n\
<li>show/qra &lt;locator&gt; [&lt;locator&gt;]\n\
</li>\n\
</ul>\n\
<p>Show distance between QRA Grid locators\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fqrz\">show/qrz</span></h3>\n\
<ul>\n\
<li>show/qrz &lt;callsign&gt;\n\
</li>\n\
</ul>\n\
<p>This command queries the QRZ callbook server on the internet and\n\
  returns any information available for that callsign. This service\n\
  is provided for users of this software by <a rel=\"nofollow\"\n\
class=\"external free\" href=\"http://www.qrz.com\">http://www.qrz.com</a>\n\
</p>\n\
<p>See also SHOW/WM7D for an alternative.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Froute\">show/route</span></h3>\n\
<ul>\n\
<li>show/route &lt;callsign&gt; ...\n\
</li>\n\
</ul>\n\
<p>This command allows you to see to which node the callsigns\n\
  specified are connected. It is a sort of inverse sh/config.\n\
</p>\n\
<pre>sh/route n2tly\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fsatellite\">show/satellite</span></h3>\n\
<ul>\n\
<li>show/satellite &lt;name&gt; [&lt;hours&gt; &lt;interval&gt;]\n\
</li>\n\
</ul>\n\
<p>Show the tracking data from your location to the satellite of\n\
  your choice from now on for the next few hours.\n\
</p>\n\
<p>If you use this command without a satellite name it will display\n\
  a list of all the satellites known currently to the system.\n\
</p>\n\
<p>If you give a name then you can obtain tracking data of all the\n\
  passes that start and finish 5 degrees below the horizon. As\n\
  default it will give information for the next three hours for\n\
  every five minute period.\n\
</p>\n\
<p>You can alter the number of hours and the step size, within\n\
  certain limits.\n\
</p>\n\
<p>Each pass in a period is separated with a row of '-----'\n\
  characters\n\
</p>\n\
<p>So for example:-\n\
</p>\n\
<pre>SH/SAT AO-10\n\
SH/SAT FENGYUN1 12 2\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fstartup\">show/startup</span></h3>\n\
<ul>\n\
<li>show/startup\n\
</li>\n\
</ul>\n\
<p>View the contents of a startup script created with SET/STARTUP.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fstation\">show/station</span></h3>\n\
<ul>\n\
<li>show/station [&lt;callsign&gt; ..]\n\
</li>\n\
</ul>\n\
<p>Show the information known about a callsign and whether (and\n\
  where) that callsign is connected to the cluster.\n\
</p>\n\
<pre>SH/ST G1TLH\n\
</pre>\n\
<p>If no callsign is given then show the information for yourself.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fsun\">show/sun</span></h3>\n\
<ul>\n\
<li>show/sun [ndays] [&lt;prefix&gt;|&lt;callsign&gt;]\n\
</li>\n\
</ul>\n\
<p>Show the sun rise and set times for a (list of) prefixes or\n\
  callsigns, together with the azimuth and elevation of the sun\n\
  currently at those locations.\n\
</p>\n\
<p>If you don't specify any prefixes or callsigns, it will show the\n\
  times for your QTH (assuming you have set it with either\n\
  SET/LOCATION or SET/QRA), together with the current azimuth and\n\
  elevation.\n\
</p>\n\
<p>If all else fails it will show the sunrise and set times for the\n\
  node that you are connected to.\n\
</p>\n\
<p>For example:-\n\
</p>\n\
<pre>SH/SUN\n\
SH/SUN G1TLH K9CW ZS\n\
</pre>\n\
<p>You can also use this command to see into the past or the future,\n\
  so if you want to see yesterday's times then do:-\n\
</p>\n\
<pre>SH/SUN -1\n\
</pre>\n\
<p>or in three days time:-\n\
</p>\n\
<pre>SH/SUN +3 W9\n\
</pre>\n\
<p>Upto 366 days can be checked both in the past and in the future.\n\
</p>\n\
<p>Please note that the rise and set times are given as the UT times\n\
  of rise and set on the requested UT day.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Ftime\">show/time</span></h3>\n\
<ul>\n\
<li>show/time [&lt;prefix&gt;|&lt;callsign&gt;]\n\
</li>\n\
</ul>\n\
<p>If no prefixes or callsigns are given then this command returns\n\
  the local time and UTC as the computer has it right now. If you\n\
  give some prefixes then it will show UTC and UTC + the local\n\
  offset (not including DST) at the prefixes or callsigns that you\n\
  specify.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fusdb\">show/usdb</span></h3>\n\
<ul>\n\
<li>show/usdb [call ..]\n\
</li>\n\
</ul>\n\
<p>Show the City and State of a Callsign held on the FCC database if\n\
  his is being run on this system, eg:-\n\
</p>\n\
<pre>sh/usdb k1xx\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fvhfstats\">show/vhfstats</span></h3>\n\
<ul>\n\
<li>show/vhfstats [days] [date]\n\
</li>\n\
</ul>\n\
<p>Show the VHF DX spots breakdown by band for the last &lt;days&gt;\n\
  no of days (default is 31), starting from a date (default: today).\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fvhftable\">show/vhftable</span></h3>\n\
<ul>\n\
<li>show/vhftable [days] [date] [prefix ...]\n\
</li>\n\
</ul>\n\
<p>Show the VHF DX Spotter table for the list of prefixes for the\n\
  last &lt;days&gt; no of days (default is 31), starting from a date\n\
  (default: today).\n\
</p>\n\
<p>If there are no prefixes then it will show the table for your\n\
  country.\n\
</p>\n\
<p>Remember that some countries have more than one \"DXCC country\" in\n\
  them (eg G&nbsp;:-), to show them (assuming you are not in G\n\
  already which is specially treated in the code) you must list all\n\
  the relevant prefixes\n\
</p>\n\
<pre>sh/vhftable g gm gd gi gj gw gu\n\
</pre>\n\
<p>Note that the prefixes are converted into country codes so you\n\
  don't have to list all possible prefixes for each country.\n\
</p>\n\
<p>If you want more or less days than the default simply include the\n\
  number you require:-\n\
</p>\n\
<pre>sh/vhftable 20 pa\n\
</pre>\n\
<p>If you want to start at a different day, simply add the date in\n\
  some recognizable form:-\n\
</p>\n\
<pre>sh/vhftable 2 25nov02\n\
sh/vhftable 2 25-nov-02\n\
sh/vhftable 2 021125\n\
sh/vhftable 2 25/11/02\n\
</pre>\n\
<p>This will show the stats for your DXCC for that CQWW contest\n\
  weekend.\n\
</p>\n\
<p>You can specify either prefixes or full callsigns (so you can see\n\
  how you did against all your mates). You can also say 'all' which\n\
  will then print the worldwide statistics.\n\
</p>\n\
<pre>sh/vhftable all\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fwcy\">show/wcy</span></h3>\n\
<p>Display the most recent WCY information that has been received by\n\
  the system\n\
</p>\n\
<ul>\n\
<li>show/wcy\n\
</li>\n\
</ul>\n\
<p>Show last 10 WCY broadcasts\n\
</p>\n\
<ul>\n\
<li>show/wcy &lt;n&gt;\n\
</li>\n\
</ul>\n\
<p>Show last &lt;n&gt; WCY broadcasts\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fwm7d\">show/wm7d</span></h3>\n\
<ul>\n\
<li>show/wm7d &lt;callsign&gt;\n\
</li>\n\
</ul>\n\
<p>This command queries the WM7D callbook server on the internet and\n\
  returns any information available for that US callsign. This\n\
  service is provided for users of this software by <a\n\
rel=\"nofollow\" class=\"external free\" href=\"http://www.wm7d.net\">http://www.wm7d.net</a>.\n\
</p>\n\
<p>See also SHOW/QRZ.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"show.2Fwwv\">show/wwv</span></h3>\n\
<p>Display the most recent WWV information that has been received by\n\
  the system\n\
</p>\n\
<ul>\n\
<li>show/wwv\n\
</li>\n\
</ul>\n\
<p>Show last 10 WWV broadcasts\n\
</p>\n\
<ul>\n\
<li>show/wwv &lt;n&gt;\n\
</li>\n\
</ul>\n\
<p>Show last &lt;n&gt; WWV broadcasts\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"sysop\">sysop</span></h3>\n\
<ul>\n\
<li>sysop\n\
</li>\n\
</ul>\n\
<p>The system automatically reduces your privilege level to that of\n\
  a normal user if you login in remotely. This command allows you to\n\
  regain your normal privilege level. It uses the normal system:\n\
  five numbers are returned that are indexes into the character\n\
  array that is your assigned password (see SET/PASSWORD). The\n\
  indexes start from zero.\n\
</p>\n\
<p>You are expected to return a string which contains the characters\n\
  required in the correct order. You may intersperse those\n\
  characters with others to obscure your reply for any watchers. For\n\
  example (and these values are for explanation&nbsp;:-):\n\
</p>\n\
<pre>password = 012345678901234567890123456789\n\
&gt; sysop\n\
22 10 15 17 3\n\
</pre>\n\
<p>you type:-\n\
</p>\n\
<pre>aa2bbbb0ccc5ddd7xxx3n\n\
or 2 0 5 7 3\n\
or 20573\n\
</pre>\n\
<p>They will all match. If there is no password you will still be\n\
  offered numbers but nothing will happen when you input a string.\n\
  Any match is case sensitive.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"talk\">talk</span></h3>\n\
<ul>\n\
<li>talk &lt;call&gt; &gt; &lt;node&gt; [&lt;text&gt;]\n\
</li>\n\
</ul>\n\
<p>Send a short message to any other station that is visible on the\n\
  cluster system. You can send it to anyone you can see with a\n\
  SHOW/CONFIGURATION command, they don't have to be connected\n\
  locally.\n\
</p>\n\
<p>The second form of TALK is used when other cluster nodes are\n\
  connected with restricted information. This usually means that\n\
  they don't send the user information usually associated with\n\
  logging on and off the cluster.\n\
</p>\n\
<p>If you know that G3JNB is likely to be present on GB7TLH, but you\n\
  can only see GB7TLH in the SH/C list but with no users, then you\n\
  would use the second form of the talk message.\n\
</p>\n\
<p>If you want to have a ragchew with someone you can leave the text\n\
  message out and the system will go into 'Talk' mode. What this\n\
  means is that a short message is sent to the recipient telling\n\
  them that you are in a go to the station that you asked for.\n\
</p>\n\
<p>All the usual announcements, spots and so on will still come out\n\
  on your terminal. If you want to do something (such as send a\n\
  spot) you preceed the normal command with a '/' character, eg:-\n\
</p>\n\
<pre>/DX 14001 G1TLH What's a B class licensee doing on 20m CW?\n\
/HELP talk\n\
</pre>\n\
<p>To leave talk mode type:\n\
</p>\n\
<pre>/EX\n\
</pre>\n\
<p>If you are in 'Talk' mode, there is an extention to the '/'\n\
  command which allows you to send the output to all the people you\n\
  are talking to. You do with the '//' command. For example:-\n\
</p>\n\
<pre>//sh/hftable\n\
</pre>\n\
<p>will send the hftable as you have it to all the people you are\n\
  currently talking to.\n\
</p>\n\
<ul>\n\
<li>talk &lt;call&gt; [&lt;text&gt;]\n\
</li>\n\
</ul>\n\
<p>Send a text message to another station\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"type\">type</span></h3>\n\
<ul>\n\
<li>type &lt;filearea&gt;/&lt;name&gt;\n\
</li>\n\
</ul>\n\
<p>Type out the contents of a file in a filearea. So, for example,\n\
  in filearea 'bulletins' you want to look at file 'arld051' you\n\
  would enter:-\n\
</p>\n\
<pre>TYPE bulletins/arld051\n\
</pre>\n\
<p>See also SHOW/FILES to see what fileareas are available and a\n\
  list of content.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fannounce\">unset/announce</span></h3>\n\
<ul>\n\
<li>unset/announce\n\
</li>\n\
</ul>\n\
<p>Stop announce messages coming out on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fanntalk\">unset/anntalk</span></h3>\n\
<ul>\n\
<li>unset/anntalk\n\
</li>\n\
</ul>\n\
<p>The announce system on legacy cluster nodes is used as a talk\n\
  substitute because the network is so poorly connected. If you:\n\
</p>\n\
<pre>unset/anntalk\n\
</pre>\n\
<p>you will suppress several of these announces, you may miss the\n\
  odd useful one as well, but you would probably miss them anyway in\n\
  the welter of useless ones.\n\
</p>\n\
<pre>set/anntalk\n\
</pre>\n\
<p>allows you to see them again. This is the default.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fbeep\">unset/beep</span></h3>\n\
<ul>\n\
<li>unset/beep\n\
</li>\n\
</ul>\n\
<p>Stop beeps for DX and other messages on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fdx\">unset/dx</span></h3>\n\
<ul>\n\
<li>unset/dx\n\
</li>\n\
</ul>\n\
<p>Stop DX messages coming out on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fdxcq\">unset/dxcq</span></h3>\n\
<ul>\n\
<li>unset/dxcq\n\
</li>\n\
</ul>\n\
<p>Stop CQ Zones on the end of DX announcements\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fdxgrid\">unset/dxgrid</span></h3>\n\
<ul>\n\
<li>unset/dxgrid\n\
</li>\n\
</ul>\n\
<p>Stop QRA Grid Square announcements </p>\n\
<p>A standard feature which is enabled in version 1.43 and above is\n\
  that if the spotter's grid square is known it is output on the end\n\
  of a DX announcement (there is just enough room). Some user\n\
  programs cannot cope with this. You can use this command to reset\n\
  (or set) this feature.\n\
</p>\n\
<p>Conflicts with: SET/DXCQ, SET/DXITU\n\
</p>\n\
<p>Do a STAT/USER to see which flags you have set if you are\n\
  confused.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fdxitu\">unset/dxitu</span></h3>\n\
<ul>\n\
<li>unset/dxitu\n\
</li>\n\
</ul>\n\
<p>Stop ITU Zones on the end of DX announcements\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fecho\">unset/echo</span></h3>\n\
<ul>\n\
<li>unset/echo\n\
</li>\n\
</ul>\n\
<p>Stop the cluster echoing your input\n\
</p>\n\
<p>If you are connected via a telnet session, different\n\
  implimentations of telnet handle echo differently depending on\n\
  whether you are connected via port 23 or some other port. You can\n\
  use this command to change the setting appropriately.\n\
</p>\n\
<p>The setting is stored in your user profile.\n\
</p>\n\
<p>YOU DO NOT NEED TO USE THIS COMMAND IF YOU ARE CONNECTED VIA\n\
  AX25.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Femail\">unset/email</span></h3>\n\
<ul>\n\
<li>unset/email\n\
</li>\n\
</ul>\n\
<p>Stop personal messages being forwarded by email\n\
</p>\n\
<p>If any personal messages come in for your callsign then you can\n\
  usevthese commands to control whether they are forwarded onto your\n\
  email address. To enable the forwarding do something like:-\n\
</p>\n\
<pre>SET/EMAIL mike.tubby@somewhere.com\n\
</pre>\n\
<p>You can have more than one email address (each one separated by a\n\
  space). Emails are forwarded to all the email addresses you\n\
  specify.\n\
</p>\n\
<p>You can disable forwarding by:-\n\
</p>\n\
<pre>UNSET/EMAIL\n\
</pre>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fhere\">unset/here</span></h3>\n\
<ul>\n\
<li>unset/here\n\
</li>\n\
</ul>\n\
<p>Tell the system you are absent from your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Flogininfo\">unset/logininfo</span></h3>\n\
<ul>\n\
<li>unset/logininfo\n\
</li>\n\
</ul>\n\
<p>No longer inform when a station logs in/out locally\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fprivilege\">unset/privilege</span></h3>\n\
<ul>\n\
<li>unset/privilege\n\
</li>\n\
</ul>\n\
<p>Remove any privilege for this session\n\
</p>\n\
<p>You can use this command to 'protect' this session from\n\
  unauthorised use. If you want to get your normal privilege back\n\
  you will need to either logout and login again (if you are on a\n\
  console) or use the SYSOP command.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fprompt\">unset/prompt</span></h3>\n\
<ul>\n\
<li>unset/prompt\n\
</li>\n\
</ul>\n\
<p>Set your prompt back to default\n\
</p>\n\
<p>UNSET/PROMPT will undo the SET/PROMPT command and set your prompt\n\
  back to normal.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fstartup\">unset/startup</span></h3>\n\
<ul>\n\
<li>unset/startup\n\
</li>\n\
</ul>\n\
<p>Remove your own startup script\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Ftalk\">unset/talk</span></h3>\n\
<ul>\n\
<li>unset/talk\n\
</li>\n\
</ul>\n\
<p>Stop TALK messages coming out on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fusstate\">unset/usstate</span></h3>\n\
<ul>\n\
<li>unset/usstate\n\
</li>\n\
</ul>\n\
<p>Stop US State info on the end of DX announcements\n\
</p>\n\
<p>If the spotter's or spotted's US State is known it is output on\n\
  the end of a DX announcement (there is just enough room).\n\
</p>\n\
<p>A spotter's state will appear on the RHS of the time (like\n\
  SET/DXGRID) and the spotted's State will appear on the LHS of the\n\
  time field. Any information found will override any locator\n\
  information from SET/DXGRID.\n\
</p>\n\
<p>Some user programs cannot cope with this. You can use this\n\
  command to reset (or set) this feature.\n\
</p>\n\
<p>Conflicts with: SET/DXCQ, SET/DXITU\n\
</p>\n\
<p>Do a STAT/USER to see which flags you have set if you are\n\
  confused.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fwcy\">unset/wcy</span></h3>\n\
<ul>\n\
<li>unset/wcy\n\
</li>\n\
</ul>\n\
<p>Stop WCY messages coming out on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fwwv\">unset/wwv</span></h3>\n\
<ul>\n\
<li>unset/wwv\n\
</li>\n\
</ul>\n\
<p>Stop WWV messages coming out on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"unset.2Fwx\">unset/wx</span></h3>\n\
<ul>\n\
<li>unset/wx\n\
</li>\n\
</ul>\n\
<p>Stop WX messages coming out on your terminal\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"who\">who</span></h3>\n\
<ul>\n\
<li>who\n\
</li>\n\
</ul>\n\
<p>Show who is physically connected\n\
</p>\n\
<p>This is a quick listing that shows which callsigns are connected\n\
  and what sort of connection they have.\n\
</p>\n\
<h3><span class=\"mw-headline\" id=\"wx\">wx</span></h3>\n\
<ul>\n\
<li>wx &lt;text&gt;\n\
</li>\n\
</ul>\n\
<p>Send a weather message to local users\n\
</p>\n\
<ul>\n\
<li>wx full &lt;text&gt;\n\
</li>\n\
</ul>\n\
<p>Send a weather message to all cluster users\n\
</p>\n\
</body>\n\
</html>";
