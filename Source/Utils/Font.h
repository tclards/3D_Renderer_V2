#pragma once

#include <string>
#include <vector>
#include "../../gateware-main/Gateware.h"
#include "tinyxml2.h"

struct Character
{
	int character;
	int x, y;
	int width, height;
	int originx, originy;
	int advance;
};

class Font
{
private:
	std::vector<Character> letters;
	std::string name;
	unsigned int size;
	unsigned int width;
	unsigned int height;
	bool bold;
	bool italic;


public:
	Font();
	~Font();
	Font(const Font& that);
	Font& operator=(const Font& that);

	bool LoadFromXML(std::string filepath);

	std::vector<Character> GetLetters() const;
	std::string GetName() const;
	unsigned int GetSize() const;
	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	bool GetBold() const;
	bool GetItalic() const;

	void SetName(std::string n);
	void SetSize(unsigned int s);
	void SetWidth(unsigned int w);
	void SetHeight(unsigned int h);
	void SetBold(bool b);
	void SetItalic(bool i);
};

struct TextVertex
{
	float pos[2];
	float uv[2];
};


class Text
{
private:
	std::string				text;
	std::vector<TextVertex> vertices;
	Font*					font;
	GW::MATH2D::GVECTOR2F	pos;
	GW::MATH2D::GVECTOR2F	scale;
	GW::MATH2D::GVECTOR2F	rot;
	UINT					texture_index;

	bool					dirty;

	TextVertex SCRtoNDC(const TextVertex& v, unsigned int w, unsigned int h);

public:
	Text();
	~Text();
	Text(const Text& that);
	Text& operator=(const Text& that);

	void Update(unsigned int w, unsigned int h);

	std::string GetText() const;
	std::vector<TextVertex> GetVertices() const;
	Font* GetFont() const;
	GW::MATH2D::GVECTOR2F GetPosition() const;
	GW::MATH2D::GVECTOR2F GetScale() const;
	float GetRotation() const;
	float GetDepth() const;
	bool GetDirtyFlag() const;


	void SetText(std::string t);
	void SetFont(Font* f);
	void SetPosition(float x, float y);
	void SetPosition(GW::MATH2D::GVECTOR2F p);
	void SetScale(float x, float y);
	void SetScale(GW::MATH2D::GVECTOR2F s);
	void SetRotation(float radian);
	void SetDepth(float depth);
	void SetDirtyFlag(bool d);
};