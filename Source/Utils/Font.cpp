#include "Font.h"
#include <iostream>

Font::Font()
{
	letters.clear();
	name = "";
	size = 0;
	width = 0;
	height = 0;
	bold = false;
	italic = false;
}

Font::~Font()
{
}

Font::Font(const Font& that)
{
	*this = that;
}

Font& Font::operator=(const Font& that)
{
	if (this != &that)
	{
		letters = that.letters;
		name = that.name;
		size = that.size;
		width = that.width;
		height = that.height;
		bold = that.bold;
		italic = that.italic;
	}
	return *this;
}

bool Font::LoadFromXML(std::string filepath)
{
	tinyxml2::XMLDocument document;
	tinyxml2::XMLError error_message = document.LoadFile(filepath.c_str());
	if (error_message != tinyxml2::XML_SUCCESS)
	{
		std::cout << "XML file [" + filepath + "] did not load properly." << std::endl;
		return false;
	}

	//name="Consolas" size="32" bold="false" italic="false" width="512" height="256">
	name = document.FirstChildElement("font")->FindAttribute("name")->Value();
	size = atoi(document.FirstChildElement("font")->FindAttribute("size")->Value());
	bold = (strcmp(document.FirstChildElement("font")->FindAttribute("bold")->Value(), "false") == 0) ? false : true;
	italic = (strcmp(document.FirstChildElement("font")->FindAttribute("italic")->Value(),"false") == 0) ? false : true;
	width = atoi(document.FirstChildElement("font")->FindAttribute("width")->Value());
	height = atoi(document.FirstChildElement("font")->FindAttribute("height")->Value());


	tinyxml2::XMLElement* current = document.FirstChildElement("font")->FirstChildElement("character");

	while (current)
	{
		Character c = Character();

		//text=" " x="380" y="139" width="3" height="3" origin-x="1" origin-y="1" advance="18"/>
		c.character = (int)(*current->FindAttribute("text")->Value());
		c.x = atoi(current->FindAttribute("x")->Value());
		c.y = atoi(current->FindAttribute("y")->Value());
		c.width = atoi(current->FindAttribute("width")->Value());
		c.height = atoi(current->FindAttribute("height")->Value());
		c.originx = atoi(current->FindAttribute("origin-x")->Value());
		c.originy = atoi(current->FindAttribute("origin-y")->Value());
		c.advance = atoi(current->FindAttribute("advance")->Value());

		letters.push_back(c);

		current = current->NextSiblingElement();
	}

	return true;
}

std::vector<Character> Font::GetLetters() const
{
	return letters;
}

std::string Font::GetName() const
{
	return name;
}

unsigned int Font::GetSize() const
{
	return size;
}

unsigned int Font::GetWidth() const
{
	return width;
}

unsigned int Font::GetHeight() const
{
	return height;
}

bool Font::GetBold() const
{
	return bold;
}

bool Font::GetItalic() const
{
	return italic;
}

void Font::SetName(std::string n)
{
	name = n;
}

void Font::SetSize(unsigned int s)
{
	size = s;
}

void Font::SetWidth(unsigned int w)
{
	width = w;
}

void Font::SetHeight(unsigned int h)
{
	height = h;
}

void Font::SetBold(bool b)
{
	bold = b;
}

void Font::SetItalic(bool i)
{
	italic = i;
}

TextVertex Text::SCRtoNDC(const TextVertex& v, unsigned int w, unsigned int h)
{
	TextVertex result = v;
	result.pos[0] = ((float)v.pos[0] / (float)w) * 2.0f - 1.0f;
	result.pos[1] = (1.0f - ((float)v.pos[1] / (float)h)) * 2.0f - 1.0f;
	return result;
}

Text::Text()
{
	text = "";
	vertices.clear();
	font = nullptr;
	dirty = false;
	pos = { 0.0f, 0.0f };
	scale = { 0.0f, 0.0f };
	rot = { 0.0f, 0.0f };
	texture_index = -1;
}

Text::~Text()
{
	text = "";
	vertices.clear();
	font = nullptr;
	dirty = false;
	pos = { 0.0f, 0.0f };
	scale = { 0.0f, 0.0f };
	rot = { 0.0f, 0.0f };
	texture_index = -1;
}

Text::Text(const Text& that)
{
	*this = that;
}

Text& Text::operator=(const Text& that)
{
	if (this != &that)
	{
		text = that.text;
		vertices = that.vertices;
		font = that.font;
		dirty = that.dirty;
		pos = that.pos;
		scale = that.scale;
		rot = that.rot;
		texture_index = that.texture_index;
	}
	return *this;
}

void Text::SetText(std::string t)
{
	if (strcmp(t.c_str(), text.c_str()) != 0)
	{
		text = t;
		dirty = true;
	}
}

void Text::SetFont(Font* f)
{
	font = f;
}

void Text::SetPosition(float x, float y)
{
	pos.x = x; pos.y = y;
}

void Text::SetPosition(GW::MATH2D::GVECTOR2F p)
{
	pos = p;
}

void Text::SetScale(float x, float y)
{
	scale.x = x; scale.y = y;
}

void Text::SetScale(GW::MATH2D::GVECTOR2F s)
{
	scale = s;
}

void Text::SetRotation(float radian)
{
	rot.x = radian;
}

void Text::SetDepth(float depth)
{
	rot.y = depth;
}

void Text::SetDirtyFlag(bool d)
{
	dirty = d;
}

void Text::Update(unsigned int w, unsigned int h)
{
	if (!font) return;
	if (dirty)
	{
		vertices.clear();

		float total_advance = 0;
		unsigned int num_characters = text.size();
		const std::vector<Character>& letters = font->GetLetters();
		float font_width = font->GetWidth();
		float font_height = font->GetHeight();
		float font_size = font->GetSize();

		for (unsigned int i = 0; i < num_characters; i++)
		{
			int index = (int)text[i] - letters[0].character;
			total_advance += (float)letters[index].advance;
		}

		//float x = -total_advance / 2;
		float x = ((float)w / 2.0f) - (total_advance / 2);
		//float y = font_size / 2;
		float y = ((float)h / 2.0f) - (font_size / 2);

		for (unsigned int i = 0; i < num_characters; i++)
		{
			int index = (int)text[i] - letters[0].character;
			Character c = letters[index];

			/*
				// p0 --- p1
				// | \     |
				// |   \   |
				// |     \ |
				// p2 --- p3
			*/

			TextVertex quad[4] = { 0 };

			quad[0].pos[0] = x - c.originx;
			quad[0].pos[1] = y - c.originy;
			quad[0].uv[0] = c.x / font_width;
			quad[0].uv[1] = c.y / font_height;

			quad[1].pos[0] = x - c.originx + c.width;
			quad[1].pos[1] = y - c.originy;
			quad[1].uv[0] = (c.x + c.width) / font_width;
			quad[1].uv[1] = c.y / font_height;

			quad[2].pos[0] = x - c.originx;
			quad[2].pos[1] = y - c.originy + c.height;
			quad[2].uv[0] = c.x / font_width;
			quad[2].uv[1] = (c.y + c.height) / font_height;

			quad[3].pos[0] = x - c.originx + c.width;
			quad[3].pos[1] = y - c.originy + c.height;
			quad[3].uv[0] = (c.x + c.width) / font_width;
			quad[3].uv[1] = (c.y + c.height) / font_height;



			vertices.push_back(SCRtoNDC(quad[2], w, h));
			vertices.push_back(SCRtoNDC(quad[0], w, h));
			vertices.push_back(SCRtoNDC(quad[3], w, h));

			vertices.push_back(SCRtoNDC(quad[0], w, h));
			vertices.push_back(SCRtoNDC(quad[1], w, h));
			vertices.push_back(SCRtoNDC(quad[3], w, h));

			x += c.advance;
		}


		dirty = false;
	}
}

std::string Text::GetText() const
{
	return text;
}

std::vector<TextVertex> Text::GetVertices() const
{
	return vertices;
}

Font* Text::GetFont() const
{
	return font;
}

GW::MATH2D::GVECTOR2F Text::GetPosition() const
{
	return pos;
}

GW::MATH2D::GVECTOR2F Text::GetScale() const
{
	return scale;
}

float Text::GetRotation() const
{
	return rot.x;
}

float Text::GetDepth() const
{
	return rot.y;
}

bool Text::GetDirtyFlag() const
{
	return dirty;
}
