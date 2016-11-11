const char* szBeginner = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\n\
    \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n\
<head>\n\
<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml; charset=UTF-8\" />\n\
<meta name=\"generator\" content=\"AsciiDoc 8.6.9\" />\n\
<title>Beginners' Guide to Fldigi</title>\n\
<style type=\"text/css\">\n\
/* Shared CSS for AsciiDoc xhtml11 and html5 backends */\n\
\n\
/* Default font. */\n\
body {\n\
  font-family: Georgia,serif;\n\
}\n\
\n\
/* Title font. */\n\
h1, h2, h3, h4, h5, h6,\n\
div.title, caption.title,\n\
thead, p.table.header,\n\
#toctitle,\n\
#author, #revnumber, #revdate, #revremark,\n\
#footer {\n\
  font-family: Arial,Helvetica,sans-serif;\n\
}\n\
\n\
body {\n\
  margin: 1em 5% 1em 5%;\n\
}\n\
\n\
a {\n\
  color: blue;\n\
  text-decoration: underline;\n\
}\n\
a:visited {\n\
  color: fuchsia;\n\
}\n\
\n\
em {\n\
  font-style: italic;\n\
  color: navy;\n\
}\n\
\n\
strong {\n\
  font-weight: bold;\n\
  color: #083194;\n\
}\n\
\n\
h1, h2, h3, h4, h5, h6 {\n\
  color: #527bbd;\n\
  margin-top: 1.2em;\n\
  margin-bottom: 0.5em;\n\
  line-height: 1.3;\n\
}\n\
\n\
h1, h2, h3 {\n\
  border-bottom: 2px solid silver;\n\
}\n\
h2 {\n\
  padding-top: 0.5em;\n\
}\n\
h3 {\n\
  float: left;\n\
}\n\
h3 + * {\n\
  clear: left;\n\
}\n\
h5 {\n\
  font-size: 1.0em;\n\
}\n\
\n\
div.sectionbody {\n\
  margin-left: 0;\n\
}\n\
\n\
hr {\n\
  border: 1px solid silver;\n\
}\n\
\n\
p {\n\
  margin-top: 0.5em;\n\
  margin-bottom: 0.5em;\n\
}\n\
\n\
ul, ol, li > p {\n\
  margin-top: 0;\n\
}\n\
ul > li     { color: #aaa; }\n\
ul > li > * { color: black; }\n\
\n\
.monospaced, code, pre {\n\
  font-family: \"Courier New\", Courier, monospace;\n\
  font-size: inherit;\n\
  color: navy;\n\
  padding: 0;\n\
  margin: 0;\n\
}\n\
pre {\n\
  white-space: pre-wrap;\n\
}\n\
\n\
#author {\n\
  color: #527bbd;\n\
  font-weight: bold;\n\
  font-size: 1.1em;\n\
}\n\
#email {\n\
}\n\
#revnumber, #revdate, #revremark {\n\
}\n\
\n\
#footer {\n\
  font-size: small;\n\
  border-top: 2px solid silver;\n\
  padding-top: 0.5em;\n\
  margin-top: 4.0em;\n\
}\n\
#footer-text {\n\
  float: left;\n\
  padding-bottom: 0.5em;\n\
}\n\
#footer-badges {\n\
  float: right;\n\
  padding-bottom: 0.5em;\n\
}\n\
\n\
#preamble {\n\
  margin-top: 1.5em;\n\
  margin-bottom: 1.5em;\n\
}\n\
div.imageblock, div.exampleblock, div.verseblock,\n\
div.quoteblock, div.literalblock, div.listingblock, div.sidebarblock,\n\
div.admonitionblock {\n\
  margin-top: 1.0em;\n\
  margin-bottom: 1.5em;\n\
}\n\
div.admonitionblock {\n\
  margin-top: 2.0em;\n\
  margin-bottom: 2.0em;\n\
  margin-right: 10%;\n\
  color: #606060;\n\
}\n\
\n\
div.content { /* Block element content. */\n\
  padding: 0;\n\
}\n\
\n\
/* Block element titles. */\n\
div.title, caption.title {\n\
  color: #527bbd;\n\
  font-weight: bold;\n\
  text-align: left;\n\
  margin-top: 1.0em;\n\
  margin-bottom: 0.5em;\n\
}\n\
div.title + * {\n\
  margin-top: 0;\n\
}\n\
\n\
td div.title:first-child {\n\
  margin-top: 0.0em;\n\
}\n\
div.content div.title:first-child {\n\
  margin-top: 0.0em;\n\
}\n\
div.content + div.title {\n\
  margin-top: 0.0em;\n\
}\n\
\n\
div.sidebarblock > div.content {\n\
  background: #ffffee;\n\
  border: 1px solid #dddddd;\n\
  border-left: 4px solid #f0f0f0;\n\
  padding: 0.5em;\n\
}\n\
\n\
div.listingblock > div.content {\n\
  border: 1px solid #dddddd;\n\
  border-left: 5px solid #f0f0f0;\n\
  background: #f8f8f8;\n\
  padding: 0.5em;\n\
}\n\
\n\
div.quoteblock, div.verseblock {\n\
  padding-left: 1.0em;\n\
  margin-left: 1.0em;\n\
  margin-right: 10%;\n\
  border-left: 5px solid #f0f0f0;\n\
  color: #888;\n\
}\n\
\n\
div.quoteblock > div.attribution {\n\
  padding-top: 0.5em;\n\
  text-align: right;\n\
}\n\
\n\
div.verseblock > pre.content {\n\
  font-family: inherit;\n\
  font-size: inherit;\n\
}\n\
div.verseblock > div.attribution {\n\
  padding-top: 0.75em;\n\
  text-align: left;\n\
}\n\
/* DEPRECATED: Pre version 8.2.7 verse style literal block. */\n\
div.verseblock + div.attribution {\n\
  text-align: left;\n\
}\n\
\n\
div.admonitionblock .icon {\n\
  vertical-align: top;\n\
  font-size: 1.1em;\n\
  font-weight: bold;\n\
  text-decoration: underline;\n\
  color: #527bbd;\n\
  padding-right: 0.5em;\n\
}\n\
div.admonitionblock td.content {\n\
  padding-left: 0.5em;\n\
  border-left: 3px solid #dddddd;\n\
}\n\
\n\
div.exampleblock > div.content {\n\
  border-left: 3px solid #dddddd;\n\
  padding-left: 0.5em;\n\
}\n\
\n\
div.imageblock div.content { padding-left: 0; }\n\
span.image img { border-style: none; vertical-align: text-bottom; }\n\
a.image:visited { color: white; }\n\
\n\
dl {\n\
  margin-top: 0.8em;\n\
  margin-bottom: 0.8em;\n\
}\n\
dt {\n\
  margin-top: 0.5em;\n\
  margin-bottom: 0;\n\
  font-style: normal;\n\
  color: navy;\n\
}\n\
dd > *:first-child {\n\
  margin-top: 0.1em;\n\
}\n\
\n\
ul, ol {\n\
    list-style-position: outside;\n\
}\n\
ol.arabic {\n\
  list-style-type: decimal;\n\
}\n\
ol.loweralpha {\n\
  list-style-type: lower-alpha;\n\
}\n\
ol.upperalpha {\n\
  list-style-type: upper-alpha;\n\
}\n\
ol.lowerroman {\n\
  list-style-type: lower-roman;\n\
}\n\
ol.upperroman {\n\
  list-style-type: upper-roman;\n\
}\n\
\n\
div.compact ul, div.compact ol,\n\
div.compact p, div.compact p,\n\
div.compact div, div.compact div {\n\
  margin-top: 0.1em;\n\
  margin-bottom: 0.1em;\n\
}\n\
\n\
tfoot {\n\
  font-weight: bold;\n\
}\n\
td > div.verse {\n\
  white-space: pre;\n\
}\n\
\n\
div.hdlist {\n\
  margin-top: 0.8em;\n\
  margin-bottom: 0.8em;\n\
}\n\
div.hdlist tr {\n\
  padding-bottom: 15px;\n\
}\n\
dt.hdlist1.strong, td.hdlist1.strong {\n\
  font-weight: bold;\n\
}\n\
td.hdlist1 {\n\
  vertical-align: top;\n\
  font-style: normal;\n\
  padding-right: 0.8em;\n\
  color: navy;\n\
}\n\
td.hdlist2 {\n\
  vertical-align: top;\n\
}\n\
div.hdlist.compact tr {\n\
  margin: 0;\n\
  padding-bottom: 0;\n\
}\n\
\n\
.comment {\n\
  background: yellow;\n\
}\n\
\n\
.footnote, .footnoteref {\n\
  font-size: 0.8em;\n\
}\n\
\n\
span.footnote, span.footnoteref {\n\
  vertical-align: super;\n\
}\n\
\n\
#footnotes {\n\
  margin: 20px 0 20px 0;\n\
  padding: 7px 0 0 0;\n\
}\n\
\n\
#footnotes div.footnote {\n\
  margin: 0 0 5px 0;\n\
}\n\
\n\
#footnotes hr {\n\
  border: none;\n\
  border-top: 1px solid silver;\n\
  height: 1px;\n\
  text-align: left;\n\
  margin-left: 0;\n\
  width: 20%;\n\
  min-width: 100px;\n\
}\n\
\n\
div.colist td {\n\
  padding-right: 0.5em;\n\
  padding-bottom: 0.3em;\n\
  vertical-align: top;\n\
}\n\
div.colist td img {\n\
  margin-top: 0.3em;\n\
}\n\
\n\
@media print {\n\
  #footer-badges { display: none; }\n\
}\n\
\n\
#toc {\n\
  margin-bottom: 2.5em;\n\
}\n\
\n\
#toctitle {\n\
  color: #527bbd;\n\
  font-size: 1.1em;\n\
  font-weight: bold;\n\
  margin-top: 1.0em;\n\
  margin-bottom: 0.1em;\n\
}\n\
\n\
div.toclevel0, div.toclevel1, div.toclevel2, div.toclevel3, div.toclevel4 {\n\
  margin-top: 0;\n\
  margin-bottom: 0;\n\
}\n\
div.toclevel2 {\n\
  margin-left: 2em;\n\
  font-size: 0.9em;\n\
}\n\
div.toclevel3 {\n\
  margin-left: 4em;\n\
  font-size: 0.9em;\n\
}\n\
div.toclevel4 {\n\
  margin-left: 6em;\n\
  font-size: 0.9em;\n\
}\n\
\n\
span.aqua { color: aqua; }\n\
span.black { color: black; }\n\
span.blue { color: blue; }\n\
span.fuchsia { color: fuchsia; }\n\
span.gray { color: gray; }\n\
span.green { color: green; }\n\
span.lime { color: lime; }\n\
span.maroon { color: maroon; }\n\
span.navy { color: navy; }\n\
span.olive { color: olive; }\n\
span.purple { color: purple; }\n\
span.red { color: red; }\n\
span.silver { color: silver; }\n\
span.teal { color: teal; }\n\
span.white { color: white; }\n\
span.yellow { color: yellow; }\n\
\n\
span.aqua-background { background: aqua; }\n\
span.black-background { background: black; }\n\
span.blue-background { background: blue; }\n\
span.fuchsia-background { background: fuchsia; }\n\
span.gray-background { background: gray; }\n\
span.green-background { background: green; }\n\
span.lime-background { background: lime; }\n\
span.maroon-background { background: maroon; }\n\
span.navy-background { background: navy; }\n\
span.olive-background { background: olive; }\n\
span.purple-background { background: purple; }\n\
span.red-background { background: red; }\n\
span.silver-background { background: silver; }\n\
span.teal-background { background: teal; }\n\
span.white-background { background: white; }\n\
span.yellow-background { background: yellow; }\n\
\n\
span.big { font-size: 2em; }\n\
span.small { font-size: 0.6em; }\n\
\n\
span.underline { text-decoration: underline; }\n\
span.overline { text-decoration: overline; }\n\
span.line-through { text-decoration: line-through; }\n\
\n\
div.unbreakable { page-break-inside: avoid; }\n\
\n\
\n\
/*\n\
 * xhtml11 specific\n\
 *\n\
 * */\n\
\n\
div.tableblock {\n\
  margin-top: 1.0em;\n\
  margin-bottom: 1.5em;\n\
}\n\
div.tableblock > table {\n\
  border: 3px solid #527bbd;\n\
}\n\
thead, p.table.header {\n\
  font-weight: bold;\n\
  color: #527bbd;\n\
}\n\
p.table {\n\
  margin-top: 0;\n\
}\n\
/* Because the table frame attribute is overriden by CSS in most browsers. */\n\
div.tableblock > table[frame=\"void\"] {\n\
  border-style: none;\n\
}\n\
div.tableblock > table[frame=\"hsides\"] {\n\
  border-left-style: none;\n\
  border-right-style: none;\n\
}\n\
div.tableblock > table[frame=\"vsides\"] {\n\
  border-top-style: none;\n\
  border-bottom-style: none;\n\
}\n\
\n\
\n\
/*\n\
 * html5 specific\n\
 *\n\
 * */\n\
\n\
table.tableblock {\n\
  margin-top: 1.0em;\n\
  margin-bottom: 1.5em;\n\
}\n\
thead, p.tableblock.header {\n\
  font-weight: bold;\n\
  color: #527bbd;\n\
}\n\
p.tableblock {\n\
  margin-top: 0;\n\
}\n\
table.tableblock {\n\
  border-width: 3px;\n\
  border-spacing: 0px;\n\
  border-style: solid;\n\
  border-color: #527bbd;\n\
  border-collapse: collapse;\n\
}\n\
th.tableblock, td.tableblock {\n\
  border-width: 1px;\n\
  padding: 4px;\n\
  border-style: solid;\n\
  border-color: #527bbd;\n\
}\n\
\n\
table.tableblock.frame-topbot {\n\
  border-left-style: hidden;\n\
  border-right-style: hidden;\n\
}\n\
table.tableblock.frame-sides {\n\
  border-top-style: hidden;\n\
  border-bottom-style: hidden;\n\
}\n\
table.tableblock.frame-none {\n\
  border-style: hidden;\n\
}\n\
\n\
th.tableblock.halign-left, td.tableblock.halign-left {\n\
  text-align: left;\n\
}\n\
th.tableblock.halign-center, td.tableblock.halign-center {\n\
  text-align: center;\n\
}\n\
th.tableblock.halign-right, td.tableblock.halign-right {\n\
  text-align: right;\n\
}\n\
\n\
th.tableblock.valign-top, td.tableblock.valign-top {\n\
  vertical-align: top;\n\
}\n\
th.tableblock.valign-middle, td.tableblock.valign-middle {\n\
  vertical-align: middle;\n\
}\n\
th.tableblock.valign-bottom, td.tableblock.valign-bottom {\n\
  vertical-align: bottom;\n\
}\n\
\n\
\n\
/*\n\
 * manpage specific\n\
 *\n\
 * */\n\
\n\
body.manpage h1 {\n\
  padding-top: 0.5em;\n\
  padding-bottom: 0.5em;\n\
  border-top: 2px solid silver;\n\
  border-bottom: 2px solid silver;\n\
}\n\
body.manpage h2 {\n\
  border-style: none;\n\
}\n\
body.manpage div.sectionbody {\n\
  margin-left: 3em;\n\
}\n\
\n\
@media print {\n\
  body.manpage div#toc { display: none; }\n\
}\n\
\n\
\n\
</style>\n\
<script type=\"text/javascript\">\n\
/*<![CDATA[*/\n\
var asciidoc = {  // Namespace.\n\
\n\
/////////////////////////////////////////////////////////////////////\n\
// Table Of Contents generator\n\
/////////////////////////////////////////////////////////////////////\n\
\n\
/* Author: Mihai Bazon, September 2002\n\
 * http://students.infoiasi.ro/~mishoo\n\
 *\n\
 * Table Of Content generator\n\
 * Version: 0.4\n\
 *\n\
 * Feel free to use this script under the terms of the GNU General Public\n\
 * License, as long as you do not remove or alter this notice.\n\
 */\n\
\n\
 /* modified by Troy D. Hanson, September 2006. License: GPL */\n\
 /* modified by Stuart Rackham, 2006, 2009. License: GPL */\n\
\n\
// toclevels = 1..4.\n\
toc: function (toclevels) {\n\
\n\
  function getText(el) {\n\
    var text = \"\";\n\
    for (var i = el.firstChild; i != null; i = i.nextSibling) {\n\
      if (i.nodeType == 3 /* Node.TEXT_NODE */) // IE doesn't speak constants.\n\
        text += i.data;\n\
      else if (i.firstChild != null)\n\
        text += getText(i);\n\
    }\n\
    return text;\n\
  }\n\
\n\
  function TocEntry(el, text, toclevel) {\n\
    this.element = el;\n\
    this.text = text;\n\
    this.toclevel = toclevel;\n\
  }\n\
\n\
  function tocEntries(el, toclevels) {\n\
    var result = new Array;\n\
    var re = new RegExp('[hH]([1-'+(toclevels+1)+'])');\n\
    // Function that scans the DOM tree for header elements (the DOM2\n\
    // nodeIterator API would be a better technique but not supported by all\n\
    // browsers).\n\
    var iterate = function (el) {\n\
      for (var i = el.firstChild; i != null; i = i.nextSibling) {\n\
        if (i.nodeType == 1 /* Node.ELEMENT_NODE */) {\n\
          var mo = re.exec(i.tagName);\n\
          if (mo && (i.getAttribute(\"class\") || i.getAttribute(\"className\")) != \"float\") {\n\
            result[result.length] = new TocEntry(i, getText(i), mo[1]-1);\n\
          }\n\
          iterate(i);\n\
        }\n\
      }\n\
    }\n\
    iterate(el);\n\
    return result;\n\
  }\n\
\n\
  var toc = document.getElementById(\"toc\");\n\
  if (!toc) {\n\
    return;\n\
  }\n\
\n\
  // Delete existing TOC entries in case we're reloading the TOC.\n\
  var tocEntriesToRemove = [];\n\
  var i;\n\
  for (i = 0; i < toc.childNodes.length; i++) {\n\
    var entry = toc.childNodes[i];\n\
    if (entry.nodeName.toLowerCase() == 'div'\n\
     && entry.getAttribute(\"class\")\n\
     && entry.getAttribute(\"class\").match(/^toclevel/))\n\
      tocEntriesToRemove.push(entry);\n\
  }\n\
  for (i = 0; i < tocEntriesToRemove.length; i++) {\n\
    toc.removeChild(tocEntriesToRemove[i]);\n\
  }\n\
\n\
  // Rebuild TOC entries.\n\
  var entries = tocEntries(document.getElementById(\"content\"), toclevels);\n\
  for (var i = 0; i < entries.length; ++i) {\n\
    var entry = entries[i];\n\
    if (entry.element.id == \"\")\n\
      entry.element.id = \"_toc_\" + i;\n\
    var a = document.createElement(\"a\");\n\
    a.href = \"#\" + entry.element.id;\n\
    a.appendChild(document.createTextNode(entry.text));\n\
    var div = document.createElement(\"div\");\n\
    div.appendChild(a);\n\
    div.className = \"toclevel\" + entry.toclevel;\n\
    toc.appendChild(div);\n\
  }\n\
  if (entries.length == 0)\n\
    toc.parentNode.removeChild(toc);\n\
},\n\
\n\
\n\
/////////////////////////////////////////////////////////////////////\n\
// Footnotes generator\n\
/////////////////////////////////////////////////////////////////////\n\
\n\
/* Based on footnote generation code from:\n\
 * http://www.brandspankingnew.net/archive/2005/07/format_footnote.html\n\
 */\n\
\n\
footnotes: function () {\n\
  // Delete existing footnote entries in case we're reloading the footnodes.\n\
  var i;\n\
  var noteholder = document.getElementById(\"footnotes\");\n\
  if (!noteholder) {\n\
    return;\n\
  }\n\
  var entriesToRemove = [];\n\
  for (i = 0; i < noteholder.childNodes.length; i++) {\n\
    var entry = noteholder.childNodes[i];\n\
    if (entry.nodeName.toLowerCase() == 'div' && entry.getAttribute(\"class\") == \"footnote\")\n\
      entriesToRemove.push(entry);\n\
  }\n\
  for (i = 0; i < entriesToRemove.length; i++) {\n\
    noteholder.removeChild(entriesToRemove[i]);\n\
  }\n\
\n\
  // Rebuild footnote entries.\n\
  var cont = document.getElementById(\"content\");\n\
  var spans = cont.getElementsByTagName(\"span\");\n\
  var refs = {};\n\
  var n = 0;\n\
  for (i=0; i<spans.length; i++) {\n\
    if (spans[i].className == \"footnote\") {\n\
      n++;\n\
      var note = spans[i].getAttribute(\"data-note\");\n\
      if (!note) {\n\
        // Use [\\s\\S] in place of . so multi-line matches work.\n\
        // Because JavaScript has no s (dotall) regex flag.\n\
        note = spans[i].innerHTML.match(/\\s*\\[([\\s\\S]*)]\\s*/)[1];\n\
        spans[i].innerHTML =\n\
          \"[<a id='_footnoteref_\" + n + \"' href='#_footnote_\" + n +\n\
          \"' title='View footnote' class='footnote'>\" + n + \"</a>]\";\n\
        spans[i].setAttribute(\"data-note\", note);\n\
      }\n\
      noteholder.innerHTML +=\n\
        \"<div class='footnote' id='_footnote_\" + n + \"'>\" +\n\
        \"<a href='#_footnoteref_\" + n + \"' title='Return to text'>\" +\n\
        n + \"</a>. \" + note + \"</div>\";\n\
      var id =spans[i].getAttribute(\"id\");\n\
      if (id != null) refs[\"#\"+id] = n;\n\
    }\n\
  }\n\
  if (n == 0)\n\
    noteholder.parentNode.removeChild(noteholder);\n\
  else {\n\
    // Process footnoterefs.\n\
    for (i=0; i<spans.length; i++) {\n\
      if (spans[i].className == \"footnoteref\") {\n\
        var href = spans[i].getElementsByTagName(\"a\")[0].getAttribute(\"href\");\n\
        href = href.match(/#.*/)[0];  // Because IE return full URL.\n\
        n = refs[href];\n\
        spans[i].innerHTML =\n\
          \"[<a href='#_footnote_\" + n +\n\
          \"' title='View footnote' class='footnote'>\" + n + \"</a>]\";\n\
      }\n\
    }\n\
  }\n\
},\n\
\n\
install: function(toclevels) {\n\
  var timerId;\n\
\n\
  function reinstall() {\n\
    asciidoc.footnotes();\n\
    if (toclevels) {\n\
      asciidoc.toc(toclevels);\n\
    }\n\
  }\n\
\n\
  function reinstallAndRemoveTimer() {\n\
    clearInterval(timerId);\n\
    reinstall();\n\
  }\n\
\n\
  timerId = setInterval(reinstall, 500);\n\
  if (document.addEventListener)\n\
    document.addEventListener(\"DOMContentLoaded\", reinstallAndRemoveTimer, false);\n\
  else\n\
    window.onload = reinstallAndRemoveTimer;\n\
}\n\
\n\
}\n\
asciidoc.install(1);\n\
/*]]>*/\n\
</script>\n\
</head>\n\
<body class=\"article\">\n\
<div id=\"header\">\n\
<h1>Beginners' Guide to Fldigi</h1>\n\
<div id=\"toc\">\n\
  <div id=\"toctitle\">Table of Contents</div>\n\
  <noscript><p><b>JavaScript must be enabled in your browser to display the table of contents.</b></p></noscript>\n\
</div>\n\
</div>\n\
<div id=\"content\">\n\
<div id=\"preamble\">\n\
<div class=\"sectionbody\">\n\
<div class=\"sidebarblock\">\n\
<div class=\"content\">\n\
<div class=\"paragraph\"><p>Of necessity, this Beginners' Guide contains only as much as you need to know to\n\
get started. You should learn how to make best use of the program by reading the\n\
<a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a>. You can also access it from within the Fldigi program from the <em>Help</em>\n\
menu item.</p></div>\n\
<div class=\"paragraph\"><p>You can install the entire html help system by downloading from Source Forge:</p></div>\n\
<div class=\"paragraph\"><p><a href=\"https://sourceforge.net/projects/fldigi/files/fldigi-help.zip/download/\">https://sourceforge.net/projects/fldigi/files/fldigi-help.zip/download/</a></p></div>\n\
<div class=\"paragraph\"><p>Unzip the downloaded file into the same folder as this document.  The menu\n\
item \"Help / Online documentation&#8230;\" will then open the local copy of the fldigi help system.</p></div>\n\
</div></div>\n\
</div>\n\
</div>\n\
<div class=\"sect1\">\n\
<h2 id=\"ref-beginners-q-a\">1. Beginners' Questions Answered</h2>\n\
<div class=\"sectionbody\">\n\
<div class=\"sect2\">\n\
<h3 id=\"_what_is_fldigi\">1.1. What is Fldigi?</h3>\n\
<div class=\"paragraph\"><p><a href=\"http://www.w1hkj.com/Fldigi.html\">Fldigi</a> is a computer program intended for Amateur Radio Digital Modes\n\
operation using a PC (Personal Computer). Fldigi operates (as does most similar\n\
software) in conjunction with a conventional HF SSB radio transceiver, and uses\n\
the PC sound card as the main means of input from the radio, and output to the\n\
radio. These are audio-frequency signals. The software also controls the radio\n\
by means of another connection, typically a serial port.</p></div>\n\
<div class=\"paragraph\"><p>Fldigi is multi-mode, which means that it is able to operate many popular\n\
digital modes without switching programs, so you only have one program to\n\
learn. Fldigi includes all the popular modes, such as DominoEX, MFSK16, PSK31,\n\
and RTTY.</p></div>\n\
<div class=\"paragraph\"><p>Unusually, Fldigi is available for multiple computer operating systems;\n\
FreeBSD&#8482;; Linux&#8482;, OS X&#8482; and Windows&#8482;.</p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_what_is_a_digital_mode\">1.2. What is a Digital Mode?</h3>\n\
<div class=\"paragraph\"><p>Digital Modes are a means of operating Amateur radio from the computer\n\
keyboard. The computer acts as <em>modem</em> (modulator - demodulator), as well as\n\
allowing you to type, and see what the other person types. It also controls the\n\
transmitter, changes modes as required, and provides various convenient features\n\
such as easy tuning of signals and prearranged messages.</p></div>\n\
<div class=\"paragraph\"><p>In this context, we are talking about modes used on the HF (high frequency)\n\
bands, specifically <em>chat</em> modes, those used to have a regular conversation in a\n\
similar way to voice or Morse, where one operator <em>talks</em> for a minute or two,\n\
then another does the same. These chat modes allow multiple operators to take\n\
part in a <em>net</em>.</p></div>\n\
<div class=\"paragraph\"><p>Because of sophisticated digital signal processing which takes place inside the\n\
computer, digital modes can offer performance that cannot be achieved using\n\
voice (and in some cases even Morse), through reduced bandwidth, improved\n\
signal-to-noise performance and reduced transmitter power requirement. Some\n\
modes also offer built-in automatic error correction.</p></div>\n\
<div class=\"paragraph\"><p>Digital Mode operating procedure is not unlike Morse operation, and many of the\n\
same abbreviations are used. Software such as Fldigi makes this very simple as\n\
most of the procedural business is set up for you using the Function Keys at the\n\
top of the keyboard. These are easy to learn.</p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_why_all_the_different_modes\">1.3. Why all the different modes?</h3>\n\
<div class=\"paragraph\"><p>HF propagation is very dependent on the ionosphere, which reflects the signals\n\
back to earth. There are strong interactions between different signals arriving\n\
from different paths. Experience has shown that particular modulation systems,\n\
speeds and bandwidths suit different operating conditions.</p></div>\n\
<div class=\"paragraph\"><p>Other factors such as available band space, operating speed and convenience,\n\
noise level, signal level and available power also affect the choice of\n\
mode. While in many cases several different modes might be suitable, having a\n\
choice adds to the operating pleasure. It is difficult to advise which mode is\n\
best for each particular occasion, and experience plays an important role.\n\
<span class=\"footnote\"><br />[To gain a good insight into each mode and its capabilities, you might\n\
consider purchasing <em>Digital Modes for All Occasions</em> (ISBN 1-872309-82-8) by\n\
Murray Greenman ZL1BPU, published by the RSGB and also available from\n\
FUNKAMATEUR and CQ Communications; or the ARRL&#8217;s <em>HF Digital Handbook</em> (ISBN\n\
0-87259-103-4) by Steve Ford, WB8IMY.]<br /></span></p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_how_do_i_recognise_and_tune_in_the_signals\">1.4. How do I recognise and tune in the signals?</h3>\n\
<div class=\"paragraph\"><p>Recognising the different modes comes with experience. It is a matter of\n\
listening to the signal, and observing the appearance of the signal on the\n\
tuning display. You can also practise transmitting with the transceiver\n\
disconnected, listening to the sound of the signals coming from the\n\
computer. There is also (see later paragraph) an automatic tuning option which\n\
can recognise and tune in most modes for you.</p></div>\n\
<div class=\"paragraph\"><p>The software provides a tuning display which shows the radio signals that are\n\
receivable within the transceiver passband. Using a <em>point and click</em> technique\n\
with the mouse, you can click on the centre of a signal to select it, and the\n\
software will tune it in for you. Some modes require more care than others, and\n\
of course you need to have the software set for the correct mode first — not\n\
always so easy!</p></div>\n\
<div class=\"paragraph\"><p>The <a href=\"#ref-rsid\">RSID</a> (automatic mode detection and tuning) feature uses a\n\
special sequence of tones transmitted at the beginning of each transmission to\n\
identify and tune in the signals received. For this feature to work, not only do\n\
you need to enable the feature in the receiver, but in addition the stations you\n\
are wishing to tune in need to have this feature enabled on transmission. Other\n\
programs also offer this RSID feature as an option.</p></div>\n\
</div>\n\
</div>\n\
</div>\n\
<div class=\"sect1\">\n\
<h2 id=\"ref-setting-up\">2. Setting Up</h2>\n\
<div class=\"sectionbody\">\n\
<div class=\"sect2\">\n\
<h3 id=\"_fldigi_settings\">2.1. Fldigi settings</h3>\n\
<div class=\"ulist\"><div class=\"title\">Essentials</div><ul>\n\
<li>\n\
<p>\n\
Use the menu <code>Configure&#8594;Operator</code> item to set the operator name, callsign,\n\
  locator and so on.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
If you have more than one sound card, use the menu <code>Configure&#8594;Sound Card</code>,\n\
  <code>Audio Devices</code> tab, to select the sound card you wish to use. You can ignore\n\
  the other tabs for now.\n\
</p>\n\
</li>\n\
</ul></div>\n\
<div class=\"ulist\"><div class=\"title\">Rig Control</div><ul>\n\
<li>\n\
<p>\n\
Use the menu <code>Configure&#8594;Rig Control</code> item to set how you will control the\n\
  rig. If you will key the rig via a serial port, in the <code>Hardware PTT</code> tab\n\
  select <em>Use serial port PTT</em>, the device name you will use, and which line\n\
  controls PTT. If in doubt, check both <em>RTS</em> and <em>DTR</em>. You <strong>must</strong> then press\n\
  the <code>Initialize</code> button.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
If you plan to use CAT control of the rig via the COM port, check <em>Use Hamlib</em>\n\
  in the <code>Hamlib</code> tab. Select your rig model from the drop-down menu and set the\n\
  serial port device name, baud rate, and RTS/CTS options as needed. If in\n\
  addition you wish to use PTT control via CAT, also check <em>PTT via Hamlib\n\
  command</em>. You <strong>must</strong> then press the <code>Initialize</code> button.\n\
</p>\n\
</li>\n\
</ul></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Note\" src=\"data:image/png;base64,\n\
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\n\
YII=\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>If your rig is CAT-capable but not yet supported by\n\
<a href=\"http://www.hamlib.org/\">Hamlib</a>, it may still be possible to control it via\n\
Fldigi&#8217;s <code>RigCAT</code> system.  Refer to the <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for details.</p></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"ulist\"><div class=\"title\">CPU Speed</div><ul>\n\
<li>\n\
<p>\n\
When you start Fldigi for the very first time, it makes a series of\n\
  measurements to determine your computer&#8217;s processing speed.  Although these\n\
  measurements are usually accurate, if you have a very slow processor (under\n\
  700MHz), you should verify that <em>Slow CPU</em> under <code>Configure&#8594;Misc&#8594;CPU</code> has\n\
  been enabled. The receiver decoding strategy of certain modems uses fewer\n\
  processor cycles in this mode.\n\
</p>\n\
</li>\n\
</ul></div>\n\
<div class=\"ulist\"><div class=\"title\">Modems</div><ul>\n\
<li>\n\
<p>\n\
Each of the modems can be individually set up from the <code>Configure&#8594;Modems</code>\n\
  multi-tabbed dialog. You need not change anything here to start with, although\n\
  it might be a good idea to set the <em>secondary text</em> for DominoEX and THOR to\n\
  something useful, such as your call and locator. <span class=\"footnote\"><br />[Secondary text is\n\
  transmitted when the text you type does not keep up with the typing speed of\n\
  the mode — this handy text appears in a small window at the very bottom of the\n\
  screen.]<br /></span> Note that this set of tabs is also where you set the RTTY modem speed\n\
  and shift, although the default values should be fine for normal operation.\n\
</p>\n\
</li>\n\
</ul></div>\n\
<div class=\"ulist\"><div class=\"title\">Other settings</div><ul>\n\
<li>\n\
<p>\n\
Use the menu <code>Configure&#8594;UI</code>, <code>Restart</code> tab, to set the aspect ratio of the\n\
  waterfall display and whether or not you want to dock a second digiscope to\n\
  the main window.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Use the menu <code>Configure&#8594;IDs</code> item to set whether you wish to transmit RSID\n\
  data at the start of each over (this is for the benefit of others and does not\n\
  affect RSID reception). If you plan to regularly use the RSID feature on\n\
  receive, you should deselect the option that starts new modems at the &#8220;sweet\n\
  spot&#8221; frequencies in <code>Misc&#8594;Sweet Spot</code>.\n\
</p>\n\
</li>\n\
</ul></div>\n\
<div class=\"paragraph\"><p>Finally, use the menu item <code>Configure&#8594;Save Config</code> to save the new\n\
configuration.</p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_sound_card_mixer\">2.2. Sound Card Mixer</h3>\n\
<div class=\"ulist\"><ul>\n\
<li>\n\
<p>\n\
Use your sound card <em>Master Volume</em> applet to select the sound card, the Wave\n\
  output and set the transmit audio level. You can check the level using the\n\
  <a href=\"#ref-tune\">Tune</a> button, top right, beyond the Menu.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
On Windows, the <em>Volume</em> applet can usually be opened by clicking\n\
  <code>Start&#8594;Run…</code> and entering <code>sndvol32</code>, or from the Control Panel.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Use your sound card <em>Recording Control</em> applet to select the sound card, the\n\
  Line or Mic input and set the receiver audio level. Watch the waterfall\n\
  display for receiver noise when setting the level. If you see any dark blue\n\
  noise, you have the right input and about the right level. The actual setting\n\
  is not very important, provided you see blue noise. If the audio level is too\n\
  high, the little diamond shaped indicator (bottom right) will show red. The\n\
  waterfall may also show red bands. Performance will be degraded if the level\n\
  is too high.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
On Windows, the <em>Record</em> applet can usually be opened by clicking\n\
  <code>Start&#8594;Run…</code> and entering <code>sndvol32</code>, or from the Control Panel. If opened\n\
  from the Control Panel, you&#8217;ll end up with the Master Volume applet, and need\n\
  to switch using <code>Options&#8594;Properties</code>, and selecting the <code>Recording</code> radio\n\
  button.\n\
</p>\n\
</li>\n\
</ul></div>\n\
</div>\n\
</div>\n\
</div>\n\
<div class=\"sect1\">\n\
<h2 id=\"ref-guided-tour\">3. Guided Tour</h2>\n\
<div class=\"sectionbody\">\n\
<div class=\"paragraph\"><p>The main window consists of three main panes.  Study it carefully as you read\n\
these notes. From top to bottom, these are the Receive pane (navajo white), the\n\
Transmit pane (light cyan), and the Waterfall pane (black). At the top is the\n\
collection of entry items which form the Log Data, and at the very top, a\n\
conventional drop-down Menu system, with entries for File, Op Mode, Configure,\n\
View and Help.</p></div>\n\
<div class=\"paragraph\"><p>Between the Transmit and the Waterfall panes is a line of boxes (buttons) which\n\
represent the Function Keys F1 - F12. This is the Macro group. Below the\n\
Waterfall pane is another line of boxes (buttons), which provide various control\n\
features. This is the Controls group. The program and various buttons can mostly\n\
be operated using the mouse or the keyboard, and users generally find it\n\
convenient to use the mouse while tuning around, and the keyboard and function\n\
keys during a QSO.</p></div>\n\
<div class=\"sect2\">\n\
<h3 id=\"ref-receive-pane\">3.1. Receive Pane</h3>\n\
<div class=\"paragraph\"><p>This is where the text from decoded incoming signals is displayed, in black\n\
text. When you transmit, the transmitted text is also displayed here, but in red,\n\
so the Receive pane becomes a complete record of the QSO. The information in\n\
this pane can also be logged to a file.</p></div>\n\
<div class=\"paragraph\"><p>The line at the bottom of this pane can be dragged up and down with the\n\
mouse. You might prefer to drag it down a bit to enlarge the Receive pane and\n\
reduce the size of the Transmit pane.</p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_transmit_pane\">3.2. Transmit Pane</h3>\n\
<div class=\"paragraph\"><p>This is where you type what you want to transmit. The mouse must click in here\n\
before you type (to obtain <em>focus</em>) otherwise your text will go nowhere. You can\n\
type in here while you are receiving, and when you start transmitting, the text\n\
already typed will be sent first. This trick is a cool way to impress others\n\
with your typing speed! As the text is transmitted, the text colour changes from\n\
black to red. At the end of the over, all the transmitted text (and any as yet\n\
not transmitted) will be deleted.</p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_waterfall_pane\">3.3. Waterfall Pane</h3>\n\
<div class=\"paragraph\"><p>This is the main tuning facility. There are three modes, Waterfall, FFT and\n\
Signal, selected by a button in the Control group. For now, leave it in\n\
Waterfall mode, as this is the easiest to tune with, and gives the best\n\
identification of the signal.</p></div>\n\
<div class=\"hdlist\"><table>\n\
<tr>\n\
<td class=\"hdlist1\">\n\
<strong><code>WF</code></strong> (Waterfall)\n\
<br />\n\
</td>\n\
<td class=\"hdlist2\">\n\
<p style=\"margin-top: 0;\">\n\
  A spectrogram display of signal strength versus frequency over passing\n\
  time. The receiver passband is analysed and displayed with lower frequencies\n\
  to the left, higher to the right. Weak signals and background noise are dark\n\
  while stronger signals show as brighter colours. As time passes (over a few\n\
  seconds), the historic signals move downwards like a waterfall.\n\
</p>\n\
</td>\n\
</tr>\n\
<tr>\n\
<td class=\"hdlist1\">\n\
<strong><code>FFT</code></strong> (Fast Fourier Transform)\n\
<br />\n\
</td>\n\
<td class=\"hdlist2\">\n\
<p style=\"margin-top: 0;\">\n\
  A spectrum display of the mean signal strength versus frequency. Again\n\
  frequency is displayed from left to right, but now the vertical direction\n\
  shows signal strength and there is no brightness or historic information.\n\
</p>\n\
</td>\n\
</tr>\n\
<tr>\n\
<td class=\"hdlist1\">\n\
<strong><code>SIG</code></strong> (Signal)\n\
<br />\n\
</td>\n\
<td class=\"hdlist2\">\n\
<p style=\"margin-top: 0;\">\n\
  An oscilloscope type of display showing the raw audio being captured by the\n\
  sound card.\n\
</p>\n\
</td>\n\
</tr>\n\
</table></div>\n\
<div class=\"paragraph\"><p>At the top of the pane is a scale of frequency in Hz, which corresponds to the\n\
frequency displayed immediately below it. This scale can be moved around and\n\
zoomed using buttons in the Control group.</p></div>\n\
<div class=\"paragraph\"><p>As you move the mouse around in this pane you will see a yellow group of tuning\n\
marks following the mouse pointer. Tuning is achieved by left-clicking on a\n\
signal displayed by the waterfall in this pane. Use these yellow marks to\n\
exactly straddle the signal and then left-click on the centre of the signal. The\n\
tuning marks change to red. The red vertical lines will show the approximate\n\
width of the active signal area (the expected signal bandwidth), while a red\n\
horizontal bar above will indicate the receiver software&#8217;s active decoding\n\
range. When you left-click, the red marks move to where you clicked, and will\n\
attempt to auto-track the signal from there.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Tip\" src=\"data:image/png;base64,\n\
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\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"title\">Audio history and &#8220;casual tuning&#8221;</div>\n\
<div class=\"paragraph\"><p>You can temporarily &#8220;monitor&#8221; a different signal by right-clicking on it. As\n\
long as you hold the mouse button down, the signal under it will be decoded; as\n\
soon as you release the mouse, decoding will revert to the previously tuned spot\n\
(where the red marks are).  If you also hold the <code>Control</code> key down before\n\
right-clicking, Fldigi will first decode all of its buffered audio at that\n\
frequency.</p></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_log_data\">3.4. Log Data</h3>\n\
<div class=\"paragraph\"><p>Fldigi provides two QSO entry views, one for casual QSO logging and the second\n\
for contesting.  The <code>View&#8594;Contest fields</code> menu item switches between the two\n\
modes.</p></div>\n\
<div class=\"paragraph\"><p>The <em>Frequency</em>, <em>Time Off</em>, and (when in contest mode) <em>#Out</em> fields are filled\n\
by the program.  All the others can be populated by manual keyboard entry or by\n\
selection from the <a href=\"#ref-receive-pane\">Receive pane</a>. The <em>Time Off</em> field is\n\
continuously updated with the current GMT time.  The <em>Time On</em> field will be\n\
filled in when the <em>Call</em> is updated, but can be modified later by the operator.</p></div>\n\
<div class=\"paragraph\"><p>A right click on the Receive pane brings up a context sensitive menu that will\n\
reflect which of the two QSO capture views you have open.  If you highlight text\n\
in the Receive pane then the menu selection will operate on that text.  If you\n\
simply point to a word of text and right click then the menu selection will\n\
operate on the single word.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Tip\" src=\"data:image/png;base64,\n\
iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAAJk0lEQVRo3u1aWWxU5xX+Zl/sGS8z\n\
4wVMbWzjEpxiF0pDgIaAoAQ1CgW3oqkqQA0vhJCgpCD6Rh7oQynKA5WoKqUyCSGKWNqmqgppTAJK\n\
EcalTkwNcYw3wPs6M559xtP//J5/dGd8Z+ba0KBIXOlovN3/nu+s3znXqmg0im/ypcY3/HoM4DGA\n\
B7y0D+ugaKwa0IcoDNICoVKp4p+Sr1WPHAApPjU1BSGRSISLAEIilCbRaDRc1Go1SZQ+HwTInAEw\n\
ZaNC4XA4jAsXLuDy5ctobm5GZ2cnRkdHEQgEYDAYYLPZUFZWhqVLl2LNmjXYuHEjdDpdXBigaAzQ\n\
rIGoZtsHhMVJ6a6uLpw4cQKnT59GXsliVCxZhsLi+ci2WmDUG5hiau6BQDCIyclJDA30ov3mDYz0\n\
tGLbtm3YvXs3ysvLOUi9Xg+tVovZemRWAMjqZPFQKITDhw/j6NGjWLftJVQtqYUl2wyXxweX24tJ\n\
rx/+YAihcIQQc6UMeh1MRj2sWWZmdTXu3rmFjz74A/bs2YNDhw4hKysLJpNJeESxNxQDIOXJ6tev\n\
X8e+ffugc1RixdPPwmwyom9oDIOjTq60NN4TRJLAJAQmz2rGnZZGjPe04MiRI1i5ciUHQh6JeUP1\n\
UMqoUP78+fPYsGEDFq7YjGfWPwePP4gbtzrR0zfMwiQENVNMnQoAhcZ0eHAJBMMMtBuO8hpUPPU8\n\
tm/fjjNnzsDpdMLn8/EQpec+cBJLlf/Fjp34+StvoqDAgc57A9zqceVkrBwvmWk8EgxFoNLnYPOO\n\
g3j9jQO8km3dupU/m0KKeSKazhNpQ4gSlpRvbGzklt/+8mHYHHZ82XEfzkmvvLKIwj02Bs+kC1Fm\n\
QIPRiDx7AYxmszwgCeiIbwIfn34LJ0+e5NUqJycHRnY/hVOqxE4LgCVslErh6tWrUc7CpryiErfa\n\
7yYon2zd0cF+RIIB7N1Vh7ofrWVxbkFrezf+dO4S7twbSu2dGBD3UA96Wxpw9uxZ2O12WK1WnhMs\n\
sVWzygEROlRtKGFJ+Q4WNk5WaaSxrJYIK7DweTx4681X8fLOrSi057PyqMN3qxfh2K9/iYoFRTMA\n\
xO+f7gPIss2HqbAKx44dg8vlypgP6nS1nuo8lcoVq9ZheNyFIRHzMknJu6xaA2t2Fn74zPdnnKln\n\
5fHF53+QukrFRG8wwlG6GPX19Whvb4eHGSTI+gjpE5UJl1QAeIelJrW+bjeyzCbc6x/JWF14vc/O\n\
hj8QlPVqZWnRDNBykm21YfmGn+LUqVO8AVIYC3qiGAA1K+qw366uQT+r86FQWFGZtOTk492/fCIL\n\
4NoX7fFwSQdEx7yQ6yjh9IQAUBiRPooAiPC5ePEi8hcsZh02G0NjrpRxr5YBcr6hCa8deRuXrt3k\n\
CU/y9rkGnsgJZ8TiXs4oBlMW7N96gvMrAUAujLSpwodurKheDjdL2hkdNqneh8Mh+Fms0kMiLOEi\n\
UxH0dnfh0qf/ojIHNSuDJWULRTnM2K3BAZhhL6lEU1MTNm3aNH0204tohrSiygIgpMQqFz39Qsp6\n\
Lx5I10h/H4od+YxlrkTlwhLMK7ShwJYHq8XMqYaOKe7yePHab+o5T8qUA3SuXm+EyWxBW9sX8Pv9\n\
0kTO3IkJKVHiZZusGOsbibsYKR6o0Wjxx98eQGlJUcqeQiROr9NmVj4mWsZOKRz7+vq48lRKSS8l\n\
OcCF+LyRNRDiLJli38gImNefWHnuMn50vP7PuPllJ//+06ZWTDCmKo17dRoAalaSickSNyLlY71A\n\
mQfoD6l0kWVDkanpBE7Bc0hsjgK0dvTCYctF8+1uNLDk/eTKNVSVzcMrO3/MLDeFc/+8npGlIomS\n\
0C9F8sopn5bMUfsWN6RLYCHv/PUy3v3wyjTTZDFLBtj/Uh3//m7/MIapkilI4DgvCk97nuYDUj65\n\
UgpulBIAjYEUexpm/aic0mk84mZur11SieXfqeJn9Q+PK459cW4kFISWRQBxIQq5BAInKUOyAOgG\n\
mmGpiRj02mnKm4oeJwFhTQReVlJ/9sK6+Hk9VAgkYQgFQIIBLzdccXFxPG/kCOmMJBYH0AA+yGZY\n\
EwullARMhhNR08k2G7F+VW38zHG3J6Fbp22EYuDxe+H3TqKiooL3D8m8nJlKULMgPt7e8m8+66az\n\
VDKbpPhf9b0nOXkTlz8QTjudJQMKhwII+jwY6L6Nmpqa+MBPeinyAClCq4/BrhY+gKsycJeE5GO1\n\
etmTixLOpPlXyZgpxOscYwrr2PP/y+dksbVQ5AGxfKLsp9VHT/stmJkC6jQPTJzIgAXzHAlnFttz\n\
0tZ86bmUvO6JEbgmhrF27Vo+kUm3FUoAqAgp3UB7m3+8dxy5FtOMcEkFRIyA0qukyJ6ZBMaAOMeH\n\
oGHW/+xv72DLli0J6xa5nZFsDoj6S0sn2tt89flVvteZEfsybNKam4uOnv6E8+YX5Ml6LxnQpHMU\n\
XtcEhnu7UFdXh9LSUmQzNiwAKKpC0jCi2KOl00jn5wh7RtOGgABC9OOz5q8QYo1IXEUshOI8KIX3\n\
/F43JgZ7WZOKsOf9B7t27eI9gADEZuJZAeBhRIlDLqSl099P/g7qsDd9EseUc3pDOP7eRxgYmWA9\n\
JIyGxlaEiZKkiHtSfri3G2qm5KUzJ7B3716+kSCh50sSWDWnrQQN17R0euNXB7B5xwEYLLZZddV0\n\
eyIKm/HB+5y8XXj/OF7f/yrn/4WFhYq2EmkXWyKZyQq0bCJOcvDgQTz7kz2w2kug0xsUdVU5IGFW\n\
bVyjg/C4x/m5H3/we6b8fl6+icbkslyi54rknfNuVKxXqMMStb169SpftVjmP4GCsmo2gOfzTYIi\n\
isDOCwcD8LjG4B4fYdVGi6H7XRjuvMHDpra2liufn5/Pwye2mcOcN3NyINxulmwTE3xvQ6uPp557\n\
EXkFJTAYs2BkcywN5FpuNQ3jMmy6i4S5tYN+HwK+SdZhvUxxDQdw5cN6Xm0oYUnhvLw8bnmLxaJI\n\
+TltpyknaFdDeUF7G1p90PbAUVqNwtIqPgZSchKpU03HDp8r+HsCBqCv6zYGOlp4k6I6T6WS4pwA\n\
0Odst9Nzfj9A3iAgxFhJaAlAA3hbWxsfAync6O8ohkkxYpVEzIjbED0gC5vNZl4mSf7v7wfk3tDQ\n\
vEAeITAk/tggQyIdAQW/IqtSSaRuLSgCydf2hibVOzKytBAxgAsA0oomaDEpm/SODF/bO7Jkb4gl\n\
gAAjPsXPpABEh6evxaekSs3pTaXqYf2zx6N6T6x6/N8qj/j6H2ll/uhtrRpgAAAAAElFTkSuQmCC\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"title\">Quick log entry</div>\n\
<div class=\"paragraph\"><p>Certain fields (<em>Call</em>, <em>Name</em>, <em>RST In</em>, <em>QTH</em> and <em>Locator</em>) may also be\n\
populated semi-automatically.  Point to a word in the Receive pane and either\n\
double-left-click or hold a Shift key down and left-click.  The program will\n\
then use some simple heuristics to decide which log field will receive the text.</p></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"paragraph\"><p>It is generally not possible to distinguish between Operator and QTH names.  For\n\
this reason, Fldigi will use the first non-Call and non-Locator word to fill the\n\
<em>Name</em> field, and subsequent clicks will send text to the <em>QTH</em> field.\n\
Likewise, a text string may be both a valid callsign and a valid\n\
<a href=\"http://en.wikipedia.org/wiki/Maidenhead_Locator_System\">IARU (Maidenhead) locator</a>.\n\
For best results, you should attempt to fill the log fields in the order in\n\
which they appear on the main window, and clear the log fields after logging the\n\
QSO.  Of course, text can always be manually typed or pasted into any of the log\n\
fields!</p></div>\n\
<div class=\"paragraph\"><p>You can query online and local (e.g. CD) database systems for data regarding a\n\
callsign.  You make the query by either clicking on the globe button, or\n\
selecting <em>Look up call</em> from the popup menu.  The latter will also move the\n\
call to the <em>Call</em> field.</p></div>\n\
<div class=\"paragraph\"><p>When the <em>Call</em> field is filled in, the logbook will be searched for the most\n\
recent QSO with that station and, if an entry is found, the <em>Name</em>, <em>QTH</em> and\n\
other fields will be pre-filled.  If the logbook dialog is open, that last QSO\n\
will also be selected for viewing in the logbook.</p></div>\n\
<div class=\"paragraph\"><p>You open the logbook by selecting from the View menu; <code>View&#8594;Logbook</code>.  The\n\
logbook title bar will show you which logbook you currently have open.  Fldigi\n\
can maintain an unlimited (except for disk space) number of logbooks.</p></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_menu\">3.5. Menu</h3>\n\
<div class=\"paragraph\"><p>At the very top of the program window is a conventional drop-down menu. If you\n\
click on any of the items, a list of optional functions will appear. Keyboard\n\
menu selection is also provided. Where underscored characters are shown in the\n\
menu, you can select these menu items from the keyboard using the marked\n\
character and <code>Alt</code> at the same time, then moving around with the\n\
<code>up</code>/<code>down</code>/<code>left</code>/<code>right</code> keys. Press <code>Esc</code> to quit from the menu with no\n\
change.</p></div>\n\
<div class=\"sect3\">\n\
<h4 id=\"_menu_functions\">3.5.1. Menu functions</h4>\n\
<div class=\"paragraph\"><div class=\"title\">File</div><p>Allows you to open or save Macros (we won&#8217;t get into that here), turn on/off\n\
logging to file, record/play audio samples, and exit the program. You can also\n\
exit the program by clicking on the <code>X</code> in the top right corner of the window,\n\
in the usual manner.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Op Mode</div><p>This is where you select the operating modem used for transmission and\n\
reception. Some modes only have one option. Where more are offered, drag the\n\
mouse down the list and sideways following the arrow to a secondary list, before\n\
releasing it. When you start the program next time, it will remember the last\n\
mode you used.</p></div>\n\
<div class=\"paragraph\"><p>Not all the modes are widely used, so choose a mode which <em>(a)</em> maximises your\n\
chance of a QSO, and <em>(b)</em> is appropriate for the band, conditions, bandwidth\n\
requirements and permissions relevant to your operating licence.</p></div>\n\
<div class=\"paragraph\"><p>At the bottom of the list are two &#8220;modes&#8221; which aren&#8217;t modes at all, and do not\n\
transmit (see <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for details). <em>WWV</em> mode allows you to receive a\n\
standard time signal so the beeps it transmits can be used for sound card\n\
calibration. <em>Freq Analysis</em> provides just a waterfall display with a very\n\
narrow cursor, and a frequency meter which indicates the received frequency in\n\
Hz to two decimal places. This is useful for on-air frequency measurement.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Configure</div><p>This is where you set up the program to suit your computer, yourself and your\n\
operating preferences. The operating settings of the program are grouped into\n\
several categories and there are menu items in which you enter your personal\n\
information, or define your computer sound card, for example. Modems can be\n\
individually changed, each having different adjustments. The Modems dialog has\n\
multiple tabs, so you can edit any one of them. Don&#8217;t fool with the settings\n\
until you know what you are doing!  The final item, <code>Save Config</code> allows you to\n\
save the altered configuration for next time you start the program (otherwise\n\
changes are temporary).</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">View</div><p>This menu item allows you to open extra windows. Most will be greyed out, but\n\
two that are available are the Digiscope, and the PSK Browser. The Digiscope\n\
provides a mode-specific graphical analysis of the received signal, and can have\n\
more than one view (left click in the new window to change the view), or maybe\n\
none at all. The PSK Browser is a rather cool tool that allows you to monitor\n\
several PSK31 signals all at the same time! These windows can be resized to\n\
suit.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Help</div><p>Brings up the Online Documentation, the Fldigi Home Page, and various\n\
information about the program.</p></div>\n\
</div>\n\
<div class=\"sect3\">\n\
<h4 id=\"_other_controls\">3.5.2. Other controls</h4>\n\
<div class=\"paragraph\" id=\"ref-rsid\"><div class=\"title\">RSID</div><p>The RxID button turns on the receive RSID (automatic mode detection and tuning)\n\
feature. When in use, the button turns yellow and no text reception is possible\n\
until a signal is identified, or the feature is turned off again. If you plan to\n\
use the RSID feature on receive, you must leave the <em>Start New Modem at Sweet\n\
Spot</em> item in the menu <code>Configure&#8594;Defaults&#8594;Misc</code> tab unchecked.</p></div>\n\
<div class=\"paragraph\" id=\"ref-tune\"><div class=\"title\">TUNE</div><p>This button transmits a continuous tone at the current audio frequency. The tone\n\
level will be at the maximum signal level for any modem, which makes this\n\
function useful for adjusting your transceiver&#8217;s output power.</p></div>\n\
</div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_macro_buttons\">3.6. Macro buttons</h3>\n\
<div class=\"paragraph\"><p>This line of buttons provides user-editable QSO features. For example, the first\n\
button on the left sends CQ for you. Both the function of these buttons (we call\n\
them Macros) and the label on each button, can be changed.</p></div>\n\
<div class=\"paragraph\"><p>Select each button to use it by pressing the corresponding Function Key (F1 -\n\
F12, you&#8217;ll notice the buttons are grouped in patterns four to a group, just as\n\
the Function Keys are). You can also select them with a left-click of the\n\
mouse. If you right-click on the button, you are able to edit the button&#8217;s label\n\
and its function. A handy dialog pops up to allow this to be done. There are\n\
many standard shortcuts, such as <code>&lt;MYCALL&gt;</code>, which you can use within the\n\
Macros. Notice that the buttons also turn the transmitter on and off as\n\
necessary.</p></div>\n\
<div class=\"paragraph\"><p>You can just about hold a complete QSO using these buttons from left to right\n\
(but please don&#8217;t!). Notice that at the right are two spare buttons you can set\n\
as you wish, and then a button labelled <code>1</code>. Yes, this is the first set of\n\
<em>four</em> sets of Macros, and you can access the others using this button, which\n\
changes to read <code>2</code>, <code>3</code>, <code>4</code> then <code>1</code> again (right-click to go backwards), or\n\
by pressing <code>Alt</code> and the corresponding number (1-4, not F1-F4) at the same\n\
time.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Note\" src=\"data:image/png;base64,\n\
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\n\
YII=\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>If you <em>really</em> mess up the Macros and can&#8217;t see how to fix them, just close the\n\
program without saving them, and reopen it.</p></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_controls\">3.7. Controls</h3>\n\
<div class=\"paragraph\"><p>The line of buttons under the waterfall is used to control the program (as\n\
opposed to the QSO). If you hover the mouse over these buttons, you&#8217;ll see a\n\
little yellow hint box appear which tells you what each button does.</p></div>\n\
<div class=\"paragraph\"><p>The first button switches between Waterfall, FFT and Scope modes. The next two\n\
buttons adjust the signal level over which the waterfall works. The default\n\
range is from 0dB downwards 70dB (i.e. to -70dB). Both of these values can be\n\
adjusted to suit your sound card and receiver audio level.</p></div>\n\
<div class=\"paragraph\"><p>The next button sets the scale zoom factor (visible display width, ×1, ×2 or\n\
×4), and the next three buttons move the visible waterfall area in relation to\n\
the bandwidth cursor.</p></div>\n\
<div class=\"paragraph\"><p>The next button selects the waterfall speed. NORM or SLOW setting is best unless\n\
you have a very fast computer.</p></div>\n\
<div class=\"paragraph\"><p>The next four buttons (two on either side of a number, the audio frequency in\n\
Hz) control the receiving frequency (they move the red cursor lines).</p></div>\n\
<div class=\"paragraph\"><p>The <code>QSY</code> button moves the signal under the bandwidth cursor to a preset audio\n\
frequency (typically, the centre of the transceiver&#8217;s passband). The Store\n\
button allows you to store or recall the current frequency and mode. See the\n\
<a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for details on these functions.</p></div>\n\
<div class=\"paragraph\"><p>The <code>Lk</code> button locks the transmit frequency (and illuminates a green marker), and the\n\
<code>Rv</code> button turns the signal decoding upside down (some modes are sideband\n\
sensitive, and if they are the wrong way up, can&#8217;t be received\n\
correctly). Remember to turn this one off when you&#8217;re done, or you won&#8217;t receive\n\
anything! If every signal you hear is upside down, check your transceiver\n\
sideband setting.</p></div>\n\
<div class=\"paragraph\"><p>The <code>T/R</code> button forces the transmitter on or off.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Caution\" src=\"data:image/png;base64,\n\
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\n\
SUVORK5CYII=\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>Use the <code>T/R</code> button with care, as it will stop transmission immediately, losing\n\
whatever is in the buffer (what you have typed in the Transmit pane), or start\n\
it immediately, even if nothing is ready to transmit.</p></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"paragraph\"><p>There are two further controls in the bottom right corner of the program, to the\n\
right of the Status line:</p></div>\n\
<div class=\"dlist\"><dl>\n\
<dt class=\"hdlist1\">\n\
<code>AFC</code> (AFC) control\n\
</dt>\n\
<dd>\n\
<p>\n\
  When this button is pressed, an indicator on the button turns yellow, and the\n\
  program will automatically retune to drifting signals. When the button is\n\
  again pressed, AFC is off, and the tuning will stay where you leave it.\n\
</p>\n\
</dd>\n\
<dt class=\"hdlist1\">\n\
<code>SQL</code> (Squelch) control\n\
</dt>\n\
<dd>\n\
<p>\n\
  When off (no coloured indicator on the button), the receiver displays all\n\
  &#8220;text&#8221; received, even if there is no signal present, and the receiver is\n\
  simply attempting to decode noise. When activated by pressing the button, the\n\
  indicator turns yellow. If the incoming signal strength exceeds that set by\n\
  the adjacent slider control (above the <code>SQL</code> button), the indicator turns\n\
  green and the incoming signal is decoded and printed. The signal strength is\n\
  indicated on the green bar beside the Squelch level slider. If nothing seems\n\
  to be printing, the first thing to do is check the Squelch!\n\
</p>\n\
</dd>\n\
</dl></div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_status_line\">3.8. Status Line</h3>\n\
<div class=\"paragraph\"><p>At the very bottom line of the Fldigi window is a row of useful information. At\n\
the left is the current operating mode. Next (some modes) is the measured\n\
signal-to-noise ratio at the receiver, and (in some modes) the measured signal\n\
intermodulation level (IMD).</p></div>\n\
<div class=\"paragraph\"><p>The larger central box shows (in DominoEX and THOR modes) the received\n\
<em>Secondary Text</em>. This is information (such as station identification) which is\n\
transmitted automatically whenever the transmitter has completed all user text\n\
that is available to send. It is transmitted using special characters, and is\n\
automatically directed to this special window. Secondary text you transmit is\n\
also shown here. This box changes size when you enlarge the program window.</p></div>\n\
</div>\n\
</div>\n\
</div>\n\
<div class=\"sect1\">\n\
<h2 id=\"ref-operating\">4. Operating</h2>\n\
<div class=\"sectionbody\">\n\
<div class=\"sect2\">\n\
<h3 id=\"_procedure\">4.1. Procedure</h3>\n\
<div class=\"paragraph\"><p>Operating procedure for digital modes is similar to that for Morse. Some of the\n\
same abbreviations are used. For example, at the beginning of an over, you might\n\
send <code>VK3XYZ de WB8ABC</code> or just <code>RR Jack</code> and so on. At the end of an over, it\n\
is usual to send <code>ZL1ABC de AA3AR K</code>, and at the end of a QSO <code>73 F3XYZ de 3D2ZZ\n\
SK</code>. When operating in a group or net it is usual to sign <code>AA3AE es gp de ZK8WW\n\
K</code>.</p></div>\n\
<div class=\"paragraph\"><p>It is also considered a courtesy to send a blank line or two (press <code>Enter</code>)\n\
before any text at the start of an over, and following the last text at the end\n\
of an over. You can also place these in the macros. The purpose is to separate\n\
your text from the previous text, and especially from any rubbish that was\n\
printed between overs.</p></div>\n\
<div class=\"paragraph\"><p>Fldigi does all of this for you. The Function Keys are set up to provide these\n\
start and end of over facilities, and can be edited to suit your preferences. In\n\
order that the other station&#8217;s callsign can appear when these keys are used, you\n\
need to set the other station&#8217;s callsign in the log data — it does not matter if\n\
you use the log facility or not.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Note\" src=\"data:image/png;base64,\n\
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\n\
YII=\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"title\">Macro symbols</div>\n\
<div class=\"paragraph\"><p>Some Function Key Macro buttons have graphic symbols on them which imply\n\
the following:</p></div>\n\
<div class=\"hdlist\"><table>\n\
<tr>\n\
<td class=\"hdlist1\">\n\
<strong><code>&gt;&gt;</code></strong>\n\
<br />\n\
</td>\n\
<td class=\"hdlist2\">\n\
<p style=\"margin-top: 0;\">\n\
The transmitter comes on and stays on when you use this button/macro.\n\
</p>\n\
</td>\n\
</tr>\n\
<tr>\n\
<td class=\"hdlist1\">\n\
<strong><code>||</code></strong>\n\
<br />\n\
</td>\n\
<td class=\"hdlist2\">\n\
<p style=\"margin-top: 0;\">\n\
The transmitter goes off when the text from this button/macro has been\n\
         sent.\n\
</p>\n\
</td>\n\
</tr>\n\
<tr>\n\
<td class=\"hdlist1\">\n\
<strong><code>&gt;|</code></strong>\n\
<br />\n\
</td>\n\
<td class=\"hdlist2\">\n\
<p style=\"margin-top: 0;\">\n\
The transmitter comes on, sends the text from this button/macro, and\n\
         goes off when the text from this button/macro has been sent.\n\
</p>\n\
</td>\n\
</tr>\n\
</table></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"paragraph\"><p>The Macros are set up to control the transmitter as necessary, but you can also\n\
switch the transmitter on at the start of an over with <code>Ctrl</code> and <code>T</code> or the TX\n\
macro button, and off again with <code>Ctrl</code> and <code>R</code> or the RX macro button. If you\n\
have Macros copied into or text already typed in the Transmit pane when you\n\
start the transmitter, this is sent first.</p></div>\n\
<div class=\"paragraph\"><p>Calling another station you have tuned in is as simple as pushing a button. Put\n\
his callsign into the log data (right click, select Call) and press the <code>ANS</code>\n\
Macro button (or F2) when you are ready. If he replies, you are in business!\n\
Then press <code>QSO</code> (F3) to start each over, and <code>BTU</code> (F4) to end it, and <code>SK</code>\n\
(F5) to sign off.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Note\" src=\"data:image/png;base64,\n\
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\n\
YII=\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>When typing text, the correct use of upper and lower case is important:</p></div>\n\
<div class=\"ulist\"><ul>\n\
<li>\n\
<p>\n\
Modes such as RTTY and THROB have no lower case capability.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
In most other modes, excessive use of upper case is considered impolite, like\n\
  SHOUTING!\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Modes such as PSK31, MFSK16, DominoEX and THOR use character sets which are\n\
  optimised for lower case. You should use lower case as much as possible in\n\
  these modes to achieve maximum text speed. In these modes upper case\n\
  characters are noticeably slower to send and also slightly more prone to\n\
  errors.\n\
</p>\n\
</li>\n\
</ul></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_adjustment\">4.2. Adjustment</h3>\n\
<div class=\"paragraph\"><p>Most digital modes do not require much transmitter power, as the receiver\n\
software is very sensitive. Many modes (PSK31, THROB, MT63) also require very\n\
high transmitter linearity, which is another reason to keep transmitter power\n\
below 30% of maximum. Some modes (Hellschreiber, Morse) have high peak power\n\
output, which may not indicate well on the conventional power meter, another\n\
reason to keep the average transmitted power low to prevent a very broad signal\n\
being transmitted.</p></div>\n\
<div class=\"paragraph\"><p>Adjust the transmitter output power using the TUNE button, top right, beyond the\n\
Menu. The output will be the same as the peak power in other modes. Adjust the\n\
master Volume applet Wave Out and Master Volume controls to achieve the\n\
appropriate power. Use of excessive drive will result in distortion (signal\n\
difficult to tune in, and often poorer reception) and a very broad signal.</p></div>\n\
<div class=\"paragraph\"><p>Some multi-carrier modes (MT63 for example) may require individual adjustment as\n\
the average power may be rather low.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Tip\" src=\"data:image/png;base64,\n\
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\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>Where possible, use the area above 1200Hz on the waterfall.</p></div>\n\
<div class=\"ulist\"><ul>\n\
<li>\n\
<p>\n\
Below 1200Hz the second harmonic of the transmitted audio will pass through\n\
  the transmitter filters.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
When using lower frequency tones, adjust the transmitter and audio level with\n\
  great care, as the second (and even third) harmonic will appear in the\n\
  transmitter passband, causing excessive signal width.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
A narrow (CW) filter in the rig is no help in this regard, as it is only used\n\
  on receive. When you do use a narrow filter, this will restrict the area over\n\
  which the receiver and transmitter will operate (without retuning of\n\
  course). Try adjusting the passband tuning (if available).\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Keep the sound card audio level to a minimum and set the transmitter gain to a\n\
  similar level used for SSB.\n\
</p>\n\
</li>\n\
</ul></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
</div>\n\
<div class=\"sect2\">\n\
<h3 id=\"_waterfall_tuning\">4.3. Waterfall Tuning</h3>\n\
<div class=\"paragraph\"><p>When using this program, as with most other digital modes programs, tuning is\n\
generally accomplished by leaving the transceiver VFO at a popular spot (for\n\
example 14.070MHz, USB), and performing all the <em>tuning</em> by moving around within\n\
the software.</p></div>\n\
<div class=\"paragraph\"><p>The Fldigi software has a second &#8220;VFO&#8221; which is tuned by clicking on the\n\
waterfall. On a busy band, you may see many signals at the same time (especially\n\
with PSK31 or Morse), and so you can click with the mouse on any one of these\n\
signals to tune it in, receive it, and if the opportunity allows, reply to the\n\
station.</p></div>\n\
<div class=\"paragraph\"><p>The software &#8220;VFO&#8221; operates in a transceive mode, so the transmitter signal is\n\
automatically and exactly tuned to the received frequency. If you click\n\
correctly on the signal, your reply will always be in tune with the other\n\
station.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Important\" src=\"data:image/png;base64,\n\
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\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>You <strong>must not</strong> use RIT (Clarifier) when using digital modes.</p></div>\n\
<div class=\"ulist\"><ul>\n\
<li>\n\
<p>\n\
With RIT on, you will probably have to retune after every over.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Use of the RIT will also cause the other station to change frequency, and you\n\
  will chase each other across the band.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Older transceivers without digital synthesis may have an unwanted offset\n\
  (frequency difference) between transmit and receive frequencies. Such rigs\n\
  should not be used for digital modes.\n\
</p>\n\
</li>\n\
</ul></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"paragraph\"><p>Wider digital modes (MT63, Olivia) can be tuned using the rig if necessary, as\n\
tuning is not at all critical. The software tuning still operates, but because\n\
the signal is so wide, there is limited ability to move around in the waterfall\n\
tuning.</p></div>\n\
</div>\n\
</div>\n\
</div>\n\
<div class=\"sect1\">\n\
<h2 id=\"ref-special-keys\">5. Special Keys</h2>\n\
<div class=\"sectionbody\">\n\
<div class=\"paragraph\"><p>Several special keyboard controls are provided to make operating easier.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Start Transmission</div><p>Press <code>Ctrl</code> and <code>T</code> to start transmission if there is text ready in the transmit\n\
buffer.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Pause Transmission</div><p>Press <code>Pause</code> or <code>Break</code> while in receive, and the program will switch to\n\
transmit mode. It will continue with the text in the transmit buffer (the\n\
Transmit pane text) from the current point, i.e. where the red (previously sent)\n\
text ends and the black (yet to be sent) text begins. If the buffer only\n\
contains unsent text, then it will begin at the first character in the\n\
buffer. If the buffer is empty, the program will switch to transmit mode, and\n\
depending on the mode of operation, will send idle characters or nothing at all\n\
until characters are entered into the buffer.</p></div>\n\
<div class=\"paragraph\"><p>If you press <code>Pause</code> or <code>Break</code> while in transmit mode, the program will return\n\
to receive mode. There may be a slight delay for some modes like MFSK, PSK and\n\
others, that requires the transmitter to send a postamble at the end of a\n\
transmission. The transmit text buffer stays intact, ready for the\n\
<code>Pause</code>/<code>Break</code> key to return you to the transmit mode .</p></div>\n\
<div class=\"paragraph\"><p>Pressing <code>Alt</code> or <code>Meta</code> and <code>R</code> has the same effect as <code>Pause</code>/<code>Break</code>. You\n\
could think of the <code>Pause</code>/<code>Break</code> key as a software break-in capability.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Escape</div><p>Pressing <code>Esc</code> while transmitting will abort the transmission. Transmission\n\
stops as soon as possible, (any necessary postamble is sent), and the program\n\
returns to receive. Any unsent text in the transmit buffer will be lost.</p></div>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img alt=\"Tip\" src=\"data:image/png;base64,\n\
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\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"paragraph\"><p>If you press <code>Esc Esc</code> (i.e. twice in quick succession), transmission stops\n\
immediately, without sending any postamble, and the program returns to\n\
receive. Any unsent text in the transmit buffer will be lost. Use this feature\n\
as an <strong>emergency stop</strong>.</p></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"paragraph\"><div class=\"title\">Return to Receive</div><p>Press <code>Ctrl</code> and <code>R</code> to insert the <code>^r</code> command in the transmit buffer at the\n\
current typing point. When transmission reaches this point, transmission will\n\
stop.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Move Typing Cursor</div><p>Press <code>Tab</code> to move the cursor (typing insertion point) to the end of the\n\
transmit buffer. This will also pause transmission. A <code>Tab</code> press at that\n\
position moves the cursor back to the character following the last one\n\
transmitted.  Morse operation is slightly different. See the <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for CW.</p></div>\n\
<div class=\"paragraph\"><div class=\"title\">Send Any ASCII Character</div><p>Press <code>Ctrl</code> and (at the same time) any three-digit number (on the numeric\n\
keypad or the normal numeric keys) to insert the ASCII character designated by\n\
that entry value into the transmit buffer. For example, <code>Ctrl 177</code> is &#8220;±&#8221;\n\
(plus/minus) and <code>Ctrl 176</code> is &#8220;°&#8221; (degree). If you press a key other than the\n\
numeric keypad&#8217;s 0-9 the sequence will be discarded.</p></div>\n\
<h2 id=\"ref-credits\" class=\"float\">Credits</h2>\n\
<div class=\"paragraph\"><p>Copyright &#169; 2008 Murray Greenman, <code>ZL1BPU</code>.</p></div>\n\
<div class=\"paragraph\"><p>Copyright &#169; 2008-2009 David Freese, <code>W1HKJ</code>.</p></div>\n\
<div class=\"paragraph\"><p>Copyright &#169; 2009 Stelios Bounanos, <code>M0GLD</code>.</p></div>\n\
<div class=\"paragraph\"><p>License GPLv3+: <a href=\"http://www.gnu.org/licenses/gpl.html\">GNU GPL version 3 or later</a>.</p></div>\n\
</div>\n\
</div>\n\
</div>\n\
<div id=\"footnotes\"><hr /></div>\n\
<div id=\"footer\">\n\
<div id=\"footer-text\">\n\
Last updated 2016-11-04 17:46:33 \n\
</div>\n\
<div id=\"footer-badges\">\n\
<a href=\"http://validator.w3.org/check?uri=referer\">\n\
  <img style=\"border:0;width:88px;height:31px\"\n\
    src=\"http://www.w3.org/Icons/valid-xhtml11-blue\"\n\
    alt=\"Valid XHTML 1.1\" height=\"31\" width=\"88\" />\n\
</a>\n\
<a href=\"http://jigsaw.w3.org/css-validator/\">\n\
  <img style=\"border:0;width:88px;height:31px\"\n\
    src=\"http://jigsaw.w3.org/css-validator/images/vcss-blue\"\n\
    alt=\"Valid CSS!\" />\n\
</a>\n\
</div>\n\
</div>\n\
</body>\n\
</html>\n\n";
