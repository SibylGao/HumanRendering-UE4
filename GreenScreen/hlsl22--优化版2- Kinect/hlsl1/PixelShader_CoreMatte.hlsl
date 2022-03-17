#include "Header.hlsli"

/*核心遮罩*/
float4 main(VertexOut pIn) : SV_TARGET
{
	float4 pixel_color=gTex_now.Sample(gSamLinear, pIn.Tex);
	//获得纹理的尺寸
	float width =gTex_now.Length.x;
	float height = gTex_now.Length.y;

/*获取纹理中的点在1920*1080分辨率中的实际坐标(xx,yy)*/
	float xx=(pIn.Tex.x*width)+(1920.0f-width)/2;
	float yy=(pIn.Tex.y*height)+(1080.0f-height)/2;

/*获得(xx,yy)点在1920*1080分辨率中的像素值或者灰度值，即是否在方形遮罩内*/
	float rad = rotationBoxCore;
	float m_cosine = cos(rad);
	float m_sine = -sin(rad);/*在这里加了一个负号，表示和opengl一样的旋转效果*/
	float m_aspectRatio = 1920.0f/1080.0f;/*1920*1080是自己设置的分辨率和软件blender中的一致*/
	float rx = xx / 1920.0f;
	float ry = yy / 1080.0f;
	float dy = (ry - yBoxCore) / m_aspectRatio;
	float dx = rx - xBoxCore;
	rx = xBoxCore + (m_cosine * dx + m_sine * dy);/*像素点经过旋转后的新位置*/
	ry = yBoxCore + (-m_sine * dx + m_cosine * dy);
	float halfHeight = heightBoxCore / 2.0f;
	float halfWidth = widthBoxCore / 2.0f;
/*以下是椭圆形的相关设置*/
	float sx = rx - xBoxCore;
	sx *= sx;
	float tx = halfWidth * halfWidth;
	float sy = ry - yBoxCore;
	sy *= sy;
	float ty = halfHeight * halfHeight;
	bool inside;
/*判断是在椭圆内部还是方块内部？*/
	bool inside_box = (rx > xBoxCore - halfWidth && rx < xBoxCore + halfWidth &&ry > yBoxCore - halfHeight && ry < yBoxCore + halfHeight);
	bool inside_ellipse = ((sx / tx) + (sy / ty)) < 1.0f;
	float tmp = step(isBoxCore, 1);
	inside = bool(tmp*int(inside_box) + (1.0 - tmp)*int(inside_ellipse));

	float value=0.0f;/*(xx,yy)点在1920*1080中的灰度值*/
	tmp = step(int(inside), 0);
	if (maskTypeCore == 0) {/*add*/
		value = (1.0 - tmp)*max(0.0f, transparentBoxCore) + tmp*0.0f;
	}
	else if (maskTypeCore == 1) {/*subtract*/
		value = (1.0 - tmp)*(0.0f - transparentBoxCore) + tmp*0.0f;
		clamp(value, 0.0f, 1.0f);
	}
	else if (maskTypeCore == 2) {/*multiply*/
		value = (1.0 - tmp)*(0.0f*transparentBoxCore) + tmp*0.0f;
	}
	else {/*not*/
		value = (1.0 - tmp)*transparentBoxCore + tmp*0.0f;
	}

	float result;

/*核心遮罩*/
	clamp(value, 0.0f, 1.0f);
	result= max(value, pixel_color.r);

	return float4(result, result, result, 1.0f);
}
//tmp = step(isGarbage, 1);
//	value = tmp*(1.0f - value) + (1.0 - tmp)*value;
//	clamp(value, 0.0f, 1.0f);
//	result= tmp*min(value, pixel_color.r) + (1.0 - tmp)*max(value, pixel_color.r);