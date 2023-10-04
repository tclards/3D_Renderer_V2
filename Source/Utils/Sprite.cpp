#include "Sprite.h"

Sprite::Sprite()
{
	name = "";
	pos = { 0.0f, 0.0f };
	scale = { 1.0f, 1.0f };
	rot = { 0.0f, 0.0f };
	texcoord_rect = { 0.0f, 0.0f, 1.0f, 1.0f };
	scissor_rect = { 0.0f, 0.0f, 0.0f, 0.0f };
	texture_index = -1;
}

Sprite::~Sprite()
{
}

Sprite::Sprite(const Sprite& that)
{
	*this = that;
}

Sprite& Sprite::operator=(const Sprite& that)
{
	if (this != &that)
	{
		name = that.name;
		pos = that.pos;
		scale = that.scale;
		rot = that.rot;
		texcoord_rect = that.texcoord_rect;
		scissor_rect = that.scissor_rect;
		texture_index = that.texture_index;
	}
	return *this;
}

std::string Sprite::GetName() const
{
	return name;
}

GW::MATH2D::GVECTOR2F Sprite::GetPosition() const
{
	return pos;
}

GW::MATH2D::GVECTOR2F Sprite::GetScale() const
{
	return scale;
}

float Sprite::GetRotation() const
{
	return rot.x;
}

float Sprite::GetDepth() const
{
	return rot.y;
}

GW::MATH2D::GRECTANGLE2F Sprite::GetTexcoordRect() const
{
	return texcoord_rect;
}

GW::MATH2D::GRECTANGLE2F Sprite::GetScissorRect() const
{
	return scissor_rect;
}

UINT Sprite::GetTextureIndex() const
{
	return texture_index;
}

void Sprite::SetName(std::string n)
{
	name = n;
}

void Sprite::SetPosition(float x, float y)
{
	pos.x = x; pos.y = y;
}

void Sprite::SetPosition(GW::MATH2D::GVECTOR2F p)
{
	pos = p;
}

void Sprite::SetScale(float x, float y)
{
	scale.x = x; scale.y = y;
}

void Sprite::SetScale(GW::MATH2D::GVECTOR2F s)
{
	scale = s;
}

void Sprite::SetRotation(float radian)
{
	rot.x = radian;
}

void Sprite::SetDepth(float depth)
{
	rot.y = depth;
}

void Sprite::SetTexcoordRect(GW::MATH2D::GRECTANGLE2F rect)
{
	texcoord_rect = rect;
}

void Sprite::SetScissorRect(GW::MATH2D::GRECTANGLE2F rect)
{
	scissor_rect = rect;
}

void Sprite::SetTextureIndex(UINT id)
{
	texture_index = id;
}
