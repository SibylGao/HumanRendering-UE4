#include "Header.hlsli"

/*gamma校正*/
float4 main(VertexOut pIn) : SV_TARGET
{
	float4 inputValue=gTex_now.Sample(gSamLinear, pIn.Tex);/*获得原始颜色值*/

	float4 output=float4(0.0,0.0,0.0,1.0);

	output[0] = inputValue[0] > 0.0f ? pow(inputValue[0], gamma) : inputValue[0];
	output[1] = inputValue[1] > 0.0f ? pow(inputValue[1], gamma) : inputValue[1];
	output[2] = inputValue[2] > 0.0f ? pow(inputValue[2], gamma) : inputValue[2];

	output[3] = inputValue[3];

	return output;
}