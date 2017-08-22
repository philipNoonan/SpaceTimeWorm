#version 330 core
//in vec3 ourColor;
in vec2 TexCoord;

const float offset = 1.0 / 300;  


layout(location = 0) out vec4 color;
layout(location = 1) out vec2 depthWeight;


// Texture samplers
uniform sampler2D _currentTextureColor;
uniform sampler2D _currentTextureDepth;
uniform sampler2D _previousTextureColor;
uniform sampler2D _previousTextureDepthWeight;

uniform int renderColor;

void main()
{

	if(renderColor == 1)
	{
	vec2 offsets[9] = vec2[](
        vec2(-offset, offset),  // top-left
        vec2(0.0f,    offset),  // top-center
        vec2(offset,  offset),  // top-right
        vec2(-offset, 0.0f),    // center-left
        vec2(0.0f,    0.0f),    // center-center
        vec2(offset,  0.0f),    // center-right
        vec2(-offset, -offset), // bottom-left
        vec2(0.0f,    -offset), // bottom-center
        vec2(offset,  -offset)  // bottom-right    
    );

		//edge detection
	//float kernel[9] = float[](
     //   0, 0, 0,
     //   0, 1, 0,
     //   0, 0, 0
	//);
	
		// blur
	float kernel[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16  
	);
	
	vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(_currentTextureDepth, TexCoord.st + offsets[i]));
    }
	
	float depthSum;

	for(int i = 0; i < 9; i++)
	{
		if(sampleTex[i].x >= 2000.0f)
		{
			sampleTex[i].x = 2000.0f;
		}
			depthSum += sampleTex[i].x;
	}
		
	float meanDepth = depthSum / 9.0f;	
		
	vec4 tempColor = texture(_currentTextureColor, TexCoord);
	//vec4 tempDepth = texture(_currentTextureDepth, TexCoord);
	vec4 tempPreviousColor = texture(_previousTextureColor, TexCoord);
	
	vec3 previousSampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        previousSampleTex[i] = vec3(texture(_previousTextureDepthWeight, TexCoord.st + (offsets[i]/1.0f)));
    }
		float previousDepthSum;

	for(int i = 0; i < 9; i++)
	{
		if(previousSampleTex[i].x >= 2000.0f)
		{
			previousSampleTex[i].x = 2000.0f;
		}
		previousDepthSum += previousSampleTex[i].x * kernel[i];
	}

		

	float previousMeanDepth = previousDepthSum;
	
	vec4 tempPreviousDepthWeight = texture(_previousTextureDepthWeight, TexCoord);
	
	//if(tempDepth.x <= 3000.0f)
	//{
		if(meanDepth <= previousMeanDepth + 10.0f) // if the pixel is closer to the camera, z is negative
		{
			color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
			depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
		}
		else
		{
			if(tempPreviousDepthWeight.y <= 0.99)
			{
				color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
				depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
			}
			else
			{
				color = vec4(mix(tempPreviousColor.xyz, tempColor.xyz, 1.0f - (tempPreviousDepthWeight.y*1.0f)), 1.0f);
				depthWeight = vec2(previousMeanDepth, tempPreviousDepthWeight.y - 0.00001f);

			}
			
		}
	//}
	
	
	}
	else
	{
		vec4 tempDepth = texture(_currentTextureDepth, TexCoord);
		color = vec4(tempDepth.x/1000.0f, tempDepth.x/1000.0f, tempDepth.x/1000.0f, 1.0f);
	
	}
    

07809781241 NAME NAME NAME deboarah FRUAT
}