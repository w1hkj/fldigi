const char* szBeginner = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\n\
    \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n\
<head>\n\
<meta http-equiv=\"Content-Type\" content=\"application/xhtml+xml; charset=UTF-8\" />\n\
<meta name=\"generator\" content=\"AsciiDoc 9.0.0rc2\" />\n\
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
/* Because the table frame attribute is overridden by CSS in most browsers. */\n\
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
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\" />\n\
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
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\" />\n\
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
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\n\
fll0/wFbNLSnMp35lwAAAABJRU5ErkJggg==\" />\n\
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
Last updated\n\
 2021-08-17 20:21:22 \n\
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
