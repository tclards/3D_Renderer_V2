#pragma once
#include "../../gateware-main/Gateware.h"

class Sprite
{
private:
	std::string name;
	GW::MATH2D::GVECTOR2F pos;
	GW::MATH2D::GVECTOR2F scale;
	GW::MATH2D::GVECTOR2F rot;
	GW::MATH2D::GRECTANGLE2F texcoord_rect;
	GW::MATH2D::GRECTANGLE2F scissor_rect;
	UINT texture_index;

public:
	Sprite();
	~Sprite();
	Sprite(const Sprite& that);
	Sprite& operator=(const Sprite& that);

	std::string GetName() const;
	GW::MATH2D::GVECTOR2F GetPosition() const;
	GW::MATH2D::GVECTOR2F GetScale() const;
	float GetRotation() const;
	float GetDepth() const;
	GW::MATH2D::GRECTANGLE2F GetTexcoordRect() const;
	GW::MATH2D::GRECTANGLE2F GetScissorRect() const;
	UINT GetTextureIndex() const;

	void SetName(std::string n);
	void SetPosition(float x, float y);
	void SetPosition(GW::MATH2D::GVECTOR2F p);
	void SetScale(float x, float y);
	void SetScale(GW::MATH2D::GVECTOR2F s);
	void SetRotation(float radian);
	void SetDepth(float depth);
	void SetTexcoordRect(GW::MATH2D::GRECTANGLE2F rect);
	void SetScissorRect(GW::MATH2D::GRECTANGLE2F rect);
	void SetTextureIndex(UINT id);

};