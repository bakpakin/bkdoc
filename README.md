# BKDoc

Better, simpler markup language for prose, notes, documentation, and more.

BKDoc is both a markup language and a commandline tool for rendering that
markup language to other document formats (currently just HTML).

BKDoc is written in C 99 for super portability and (eventually) speed. The
parser aims to be safe, fast, and small.

## Install

Clone the repository, make, and make install. You can also build and install with the
CMake based build. Installation is by default in /usr/local/bin.

### Make
```bash
git clone https://github.com/bakpakin/bkdoc.git
cd bkdoc
make && make install
```

### CMake
```bash
git clone https://github.com/bakpakin/bkdoc.git
cd bkdoc
mkdir build && cd build
cmake ..
make && make install
```

This creates a binary `./bkd`, which reads from stdin and converts the BKDoc markup into HTML,
which is sent to stdout. You can parse a file like so (from your build directory)

```bash
./bkd < in.bkd > out.html
```

This syntax will probably change as options are added and the command line tool is made more robust.

## Why

I needed a fast markup language that I could use to generate beautiful documents
in a number of different formats. I have been using Pandoc + Markdown for this task, but
it has a number of annoying problems that are inherent in its design.

* **Markdown is missing features** - Markdown is easy to read and write, but It is missing a number of useful
  features that I like when I take notes or write documentation, such as inline diagrams, graphs, alphabetical lists,
  and more.
* **It's slow** - not really slow, but generating PDFs and even HTML takes too long. Compilation often
  fails and I have to recompile again.
* **It's bloated** - Again, the core of pandoc isn't huge, but since it has a lot of functionality
  it is bigger than I would want, which is to write Markdown and output documents.
  Also, the LaTeX requirement for PDFs is a real annoyance.
* **The HTML output isn't standalone** - The html files that pandoc outputs can't really be standalone
  with embedded math.
* **Different out formats display differently** - By this, I mean that certain LaTex math features may work
  in HTML pages but not in PDFs. This is annoying, but expected considering the scope of pandoc.

As BKDoc currently stands, I'm still using Pandoc for notes. For example, BKDoc right now has similar features
to vanilla markdown and conspicuously lacks math, diagrams, and out formats other than HTML. But it is still a fast simple tool
for your markup needs. I'm not sure if formal Latex like math will ever make it into bkdoc.

## How it works

The parser first parses your markup and converts it into a syntax tree, represented by
the structures in `include/bkd.h'. The program then outputs the tree to HTML. To add new
formats, we just need to convert the syntax tree to the output format.

## Syntax

This syntax is not set in stone. Rather than a formal specification, an example is probably helpful.

```
# BKDoc

## Titles are similar to markdown, and can be multi level.

Chunks of texts are paragraphs.
There is no new line in a paragraph
unless you explicitly use the \\n escape.\n
This is on a new line.

* You can do lists
* list items
  can be on multiple lines by indenting 2 spaces.
  Note that tabs are equal to 4 spaces.

  You can also have multiple paragraphs and other lists
  inside a list item.
  % This is a nested, numbered list.
  % The numbers are generated for you, you cannot manually set the numbers.

+ There are 6 list types. This is lowercase roman numerals.
+ A @ is an upper-case lettered list, and a & is a lowercase lettered list.
+ Uppercase roman numerals is _.


You can also markup chunks of text. [B:This is bold], [I:this is italicized],
and [BI:this is bold and italicized]. You can [B: nest [I: these]].

You can also add images, [P](https://upload.wikimedia.org/wikipedia/commons/e/ed/Ara_macao_-on_a_small_bicycle-8.jpg),
[L:name links](https://www.google.com/), and inline code [C:var hi = "Hello, World!";].

BKDoc expects UTF-8 input. You can embed Unicode emojis and CJK
characters directly into your markup, or use ASCII escapes for
unicode. For example, \(263A) is a unicode smily face.

<div>BKDoc is also 100% HTML safe, unlike Markdown. You cannot embed HTML into
a document and have it rendered.</div>

Next is a line break.

---------------------------------

Anchors and internal links are similar to HTML.
[A:This is an anchor](anchor-1)

[#:This is an internal link](anchor-1)

Code block syntax with 3 back ticks is the same as markdown.

> Email style comment blocks
> are currently the same as markdown.\n
> Other metadata may be needed eventually, such
> as the person quoted, etc.

```

There are a number of features I would like to add. Some are easy and will
definitely be added soon, others are more difficult and maybe added later.

- [X] Block comments (easy)
- [ ] Inline tables, like with Github markdown
- [X] More list styles, like roman numerals, or maybe eventually with pattern matching.
- [ ] More verbose syntax for tables to allow complex cells.
- [X] Internal linking and reference system. Preferably more general than that of Markdown.
- [X] Code language marker (easy)
- [ ] Metadata - custom tags on lists and other block elements.
- [ ] Math expressions, likely in conjuction or based on MathML (hard)
- [ ] Inline diagram syntax - UML and or generic boxes, lines, and text diagrams fromASCII. NOT a conversion of ASCII art to images (hard)
- [ ] Text macros / variables - especially useful for repeated math expressions, as well as separating links from their paths.
- [ ] More output formats besides HTML - We just have to render the syntax tree to a new format.
