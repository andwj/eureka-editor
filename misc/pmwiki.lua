-- This is a custom writer for pandoc producing pmWiki format.
-- Created by Andrew Apted, May 2018, based on sample.lua.
--
-- Invoke with: pandoc -t pmwiki.lua

-- Character escaping
local function escape(s, in_attribute)
    return s:gsub("[<>&\"']",
    function(x)
      if x == '<' then
        return '&lt;'
      elseif x == '>' then
        return '&gt;'
      elseif x == '&' then
        return '&amp;'
      elseif x == '"' then
        return '&quot;'
      elseif x == "'" then
        return '&#39;'
      else
        return x
      end
    end)
end

-- Helper function to convert an attributes table into
-- a string that can be put into HTML tags.
local function attributes(attr)
  local attr_table = {}
  for x,y in pairs(attr) do
    if y and y ~= "" then
      table.insert(attr_table, ' ' .. x .. '="' .. escape(y,true) .. '"')
    end
  end
    return table.concat(attr_table)
end

-- Table to store footnotes, so they can be included at the end.
local notes = {}

-- Blocksep is used to separate block elements.
function Blocksep()
    return "\n\n"
end

-- This function is called once for the whole document. Parameters:
-- body is a string, metadata is a table, variables is a table.
-- This gives you a fragment.  You could use the metadata table to
-- fill variables in a custom lua template.  Or, pass `--template=...`
-- to pandoc, and pandoc will add do the template processing as
-- usual.
function Doc(body, metadata, variables)
  local buffer = {}
  local function add(s)
    table.insert(buffer, s)
  end
  add(body)
  if #notes > 0 then
    add('<ol class="footnotes">')
    for _,note in pairs(notes) do
      add(note)
    end
    add('</ol>')
  end
    return table.concat(buffer,'\n') .. '\n'
end

-- The functions that follow render corresponding pandoc elements.
-- s is always a string, attr is always a table of attributes, and
-- items is always an array of strings (the items in a list).
-- Comments indicate the types of other variables.

function Str(s)
    return escape(s)
end

function Space()
    return " "
end

function SoftBreak()
    return "\n"
end

function LineBreak()
    return "<br/>"
end

function Emph(s)
    return "<em>" .. s .. "</em>"
end

function Strong(s)
    return "<strong>" .. s .. "</strong>"
end

function Subscript(s)
    return "<sub>" .. s .. "</sub>"
end

function Superscript(s)
    return "<sup>" .. s .. "</sup>"
end

function SmallCaps(s)
    return '<span style="font-variant: small-caps;">' .. s .. '</span>'
end

function Strikeout(s)
    return '<del>' .. s .. '</del>'
end

function Link(s, src, tit, attr)
    return "<a href='" .. escape(src,true) .. "' title='" ..
         escape(tit,true) .. "'>" .. s .. "</a>"
end

function Image(s, src, tit, attr)
    return "<img src='" .. escape(src,true) .. "' title='" ..
         escape(tit,true) .. "'/>"
end

function Code(s, attr)
    return "<code" .. attributes(attr) .. ">" .. escape(s) .. "</code>"
end

function InlineMath(s)
    return "\\(" .. escape(s) .. "\\)"
end

function DisplayMath(s)
    return "\\[" .. escape(s) .. "\\]"
end

function Note(s)
  local num = #notes + 1
  -- insert the back reference right before the final closing tag.
  s = string.gsub(s,
          '(.*)</', '%1 <a href="#fnref' .. num ..  '">&#8617;</a></')
  -- add a list item with the note to the note table.
  table.insert(notes, '<li id="fn' .. num .. '">' .. s .. '</li>')
  -- return the footnote reference, linked to the note.
    return '<a id="fnref' .. num .. '" href="#fn' .. num ..
            '"><sup>' .. num .. '</sup></a>'
end

function Span(s, attr)
    return "<span" .. attributes(attr) .. ">" .. s .. "</span>"
end

function RawInline(format, str)
  if format == "html" then
    return str
  end
end

function Cite(s, cs)
  local ids = {}
  for _,cit in ipairs(cs) do
    table.insert(ids, cit.citationId)
  end
    return "<span class=\"cite\" data-citation-ids=\"" .. table.concat(ids, ",") ..
    "\">" .. s .. "</span>"
end

function Plain(s)
    return s
end

function Para(s)
    return "<p>" .. s .. "</p>"
end

-- lev is an integer, the header level.
function Header(lev, s, attr)
    if lev <= 1 then
        return "(:notitle:)\n" .. "!" .. s
    elseif lev <= 2 then
        return "!!" .. s
    else
        return "!!!" .. s
    end
end

function BlockQuote(s)
    return "<blockquote>\n" .. s .. "\n</blockquote>"
end

function HorizontalRule()
    return "----"
end

function CodeBlock(s, attr)
    return "<pre><code" .. attributes(attr) .. ">" .. escape(s) ..
           "</code></pre>"
end

function BulletList(items)
  local buffer = {}
  for _, item in pairs(items) do
    table.insert(buffer, "<li>" .. item .. "</li>")
  end
    return "<ul>\n" .. table.concat(buffer, "\n") .. "\n</ul>"
end

function OrderedList(items)
  local buffer = {}
  for _, item in pairs(items) do
    table.insert(buffer, "<li>" .. item .. "</li>")
  end
    return "<ol>\n" .. table.concat(buffer, "\n") .. "\n</ol>"
end

-- Revisit association list STackValue instance.
function DefinitionList(items)
  local buffer = {}
  for _,item in pairs(items) do
    for k, v in pairs(item) do
      table.insert(buffer,"<dt>" .. k .. "</dt>\n<dd>" ..
                        table.concat(v,"</dd>\n<dd>") .. "</dd>")
    end
  end
    return "<dl>\n" .. table.concat(buffer, "\n") .. "\n</dl>"
end

-- Convert pandoc alignment to something HTML can use.
-- align is AlignLeft, AlignRight, AlignCenter, or AlignDefault.
function html_align(align)
  if align == 'AlignLeft' then
    return 'left'
  elseif align == 'AlignRight' then
    return 'right'
  elseif align == 'AlignCenter' then
    return 'center'
  else
    return 'left'
  end
end

function CaptionedImage(src, tit, caption, attr)
   return '<div class="figure">\n<img src="' .. escape(src,true) ..
      '" title="' .. escape(tit,true) .. '"/>\n' ..
      '<p class="caption">' .. caption .. '</p>\n</div>'
end

-- Caption is a string, aligns is an array of strings,
-- widths is an array of floats, headers is an array of
-- strings, rows is an array of arrays of strings.
function Table(caption, aligns, widths, headers, rows)
    -- TODO
    return "\n"
end

function RawBlock(format, str)
  if format == "html" then
    return str
  end
end

function Div(s, attr)
    return "<div" .. attributes(attr) .. ">\n" .. s .. "</div>"
end

-- The following code will produce runtime warnings when you haven't defined
-- all of the functions you need for the custom writer, so it's useful
-- to include when you're working on a writer.
local meta = {}
meta.__index =
  function(_, key)
    io.stderr:write(string.format("WARNING: Undefined function '%s'\n",key))
    return function() return "" end
  end
setmetatable(_G, meta)
