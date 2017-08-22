#version 330 core
//in vec3 ourColor;
in vec2 TexCoord;

const float offsetFS = 1.0f / 900.0f;  


layout(location = 0) out vec4 color;
layout(location = 1) out vec2 depthWeight;


// Texture samplers
uniform sampler2D _currentTextureColor;
uniform sampler2D _currentTextureDepth;
uniform sampler2D _currentTextureFlow;
uniform sampler2D _previousTextureColor;
uniform sampler2D _previousTextureDepthWeight;

uniform float decayValue;
uniform float decayThreshold;
uniform float depthThresh;
uniform int renderColor;
uniform vec2      tcOffset[25]; // Texture coordinate offsets
uniform int effectType;


void main()
{


	
	vec2 offsetsFS[9] = vec2[](
        vec2(-offsetFS, offsetFS),  // top-left
        vec2(0.0f,    offsetFS),  // top-center
        vec2(offsetFS,  offsetFS),  // top-right
        vec2(-offsetFS, 0.0f),    // center-left
        vec2(0.0f,    0.0f),    // center-center
        vec2(offsetFS,  0.0f),    // center-right
        vec2(-offsetFS, -offsetFS), // bottom-left
        vec2(0.0f,    -offsetFS), // bottom-center
        vec2(offsetFS,  -offsetFS)  // bottom-right    
    );
	
	if(renderColor == 1)
	{


		//edge detection
	//float kernel[9] = float[](
     //   0, 0, 0,
     //   0, 1, 0,
     //   0, 0, 0
	//);
	
		// blur
	float kernel[9] = float[](
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f,
    2.0f / 16.0f, 4.0f / 16.0f, 2.0f / 16.0f,
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f  
	);
	
	vec4 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec4(texture(_currentTextureDepth, TexCoord + offsetsFS[i]));
    }
	
	float depthSum = 0.0f;
	
	for(int i = 0; i < 9; i++)
	{
		if(sampleTex[i].x >= 4000.0f)
		{
			depthSum += 4000.0f;

		}
		
		depthSum += sampleTex[i].x;

	}
		
	float meanDepth = depthSum / 9.0f;	
	
	//float meanDepth = sampleTex[4].x;	


	
	//vec4 sampleTexColor = vec4(0.0f);
    //for(int i = 0; i < 9; i++)
    //{
    //    sampleTexColor += vec4(texture(_currentTextureColor, TexCoord + offsetsFS[i]));
    //}
	
	
	//vec4 tempColor = vec4(sampleTexColor.x/9.0f, sampleTexColor.y/9.0f, sampleTexColor.z/9.0f, 1.0f);
		vec4 tempColor = texture(_currentTextureColor, TexCoord);

	vec4 gaussBlurColor;
	vec4 sample[25];
 
		for (int i = 0; i < 25; i++)
		{
			// Sample a grid around and including our texel
			sample[i] = texture(_currentTextureColor, TexCoord + tcOffset[i]);
		}
 
		// Gaussian weighting:
		// 1  4  7  4 1
		// 4 16 26 16 4
		// 7 26 41 26 7 / 273 (i.e. divide by total of weightings)
		// 4 16 26 16 4
		// 1  4  7  4 1
 
    	gaussBlurColor = (
        	      (1.0  * (sample[0] + sample[4]  + sample[20] + sample[24])) +
	              (4.0  * (sample[1] + sample[3]  + sample[5]  + sample[9] + sample[15] + sample[19] + sample[21] + sample[23])) +
	              (7.0  * (sample[2] + sample[10] + sample[14] + sample[22])) +
	              (16.0 * (sample[6] + sample[8]  + sample[16] + sample[18])) +
	              (26.0 * (sample[7] + sample[11] + sample[13] + sample[17])) +
	              (41.0 * sample[12])
	               ) / 273.0;
 
	//vec4 tempDepth = texture(_currentTextureDepth, TexCoord);
	
		vec4 gaussBlurColorPrev;
	vec4 samplePrev[25];
 
		for (int i = 0; i < 25; i++)
		{
			// Sample a grid around and including our texel
			samplePrev[i] = texture(_previousTextureColor, TexCoord + tcOffset[i]);
		}
 
		// Gaussian weighting:
		// 1  4  7  4 1
		// 4 16 26 16 4
		// 7 26 41 26 7 / 273 (i.e. divide by total of weightings)
		// 4 16 26 16 4
		// 1  4  7  4 1
 
    	gaussBlurColorPrev = (
        	      (1.0  * (samplePrev[0] + samplePrev[4]  + samplePrev[20] + samplePrev[24])) +
	              (4.0  * (samplePrev[1] + samplePrev[3]  + samplePrev[5]  + samplePrev[9] + samplePrev[15] + samplePrev[19] + samplePrev[21] + samplePrev[23])) +
	              (7.0  * (samplePrev[2] + samplePrev[10] + samplePrev[14] + samplePrev[22])) +
	              (16.0 * (samplePrev[6] + samplePrev[8]  + samplePrev[16] + samplePrev[18])) +
	              (26.0 * (samplePrev[7] + samplePrev[11] + samplePrev[13] + samplePrev[17])) +
	              (41.0 * samplePrev[12])
	               ) / 273.0;
 
	//vec4 tempDepth = texture(_currentTextureDepth, TexCoord);
	
	//vec4 tempPreviousColor = texture(_previousTextureColor, TexCoord);
	
	
	vec4 previousSampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        previousSampleTex[i] = vec4(texture(_previousTextureDepthWeight, TexCoord + offsetsFS[i]));
    }
	
	float previousDepthSum = 0.0f;
	for(int i = 0; i < 9; i++)
	{
		if(previousSampleTex[i].x >= 4000.0f)
		{
			previousDepthSum += 4000.0f * kernel[i];

		}
		else
		{
			previousDepthSum += (previousSampleTex[i].x * kernel[i]);
		}
	}

		
	float previousMeanDepth = previousDepthSum;// / numberOfValidPixels++; // DONT NEED TO DIVIDE BY NUMBER OF PIXELS THE KERNEL IS PRENORMED

	//float previousMeanDepth = previousSampleTex[4].x;
	
	vec4 tempPreviousDepthWeight = vec4(texture(_previousTextureDepthWeight, TexCoord));
	
	
	
	
	
	
	if (effectType == 0)
	{
	if (tempPreviousDepthWeight.y == 1.0f)
	{
		if (meanDepth > depthThresh)
		{
			color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
			depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
		}		
		else
		{
			color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
			depthWeight = vec2(sampleTex[4].x, 0.99f); // i.e. max weigth
		}
	}
	else
	{
		if (sampleTex[4].x < previousMeanDepth + 50.0f) // if closer to the camera
		{
			color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
			depthWeight = vec2(tempPreviousDepthWeight.x + 10.0f, tempPreviousDepthWeight.y - 0.0001f); // i.e. max weigth
		}
		else
		{
		

			
//			if (tempPreviousDepthWeight.y < decayThreshold) // we are background, and below decay threshold
//			{
//				if (meanDepth < previousMeanDepth + 10.0f )
//				{
//							color = vec4(gaussBlurColorPrev.xyz, 1.0f);
//
//				//color = vec4(mix(gaussBlurColorPrev.xyz, tempColor.xyz, tempPreviousDepthWeight.y / 10000.0f), 1.0f);
//				depthWeight = vec2(previousMeanDepth + (decayValue * 0.0001f), tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth
//
//				}
//				else
//				{
//				
//					if (meanDepth <= 4000.0f)
//					{
//						color = vec4(gaussBlurColorPrev.xyz, 1.0f);
//
//						//color = vec4(mix(gaussBlurColorPrev.xyz, tempColor.xyz, tempPreviousDepthWeight.y / 10000.0f), 1.0f);
//						depthWeight = vec2(previousMeanDepth + (decayValue * 0.001f), tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth
//
//					}
//					else
//					{
//											color = vec4(gaussBlurColorPrev.xyz, 1.0f);
//
//						//color = vec4(mix(gaussBlurColorPrev.xyz, vec3(0.0f,0.0f,0.0f), tempPreviousDepthWeight.y / 10000.0f), 1.0f);
//						depthWeight = vec2(previousMeanDepth + (decayValue * 0.001f), tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth
//					}
//					
//				
//				}
//				
//				if (tempPreviousDepthWeight.y < (decayThreshold - 0.1f))
//				{
//					if (meanDepth <= 4000.0f)
//					{
//						color = tempColor; // reset
//						depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
//					}
//					else
//					{
//						color = vec4(0.0f, 0.0f, 0.0f, 0.5f); // reset
//						depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
//					}
//					
//				}				
//
//
//			}
//			else
//			{
				//color = vec4(gaussBlurColorPrev.xyz, 1.0f);
				color = samplePrev[12];
				//color = vec4(mix(gaussBlurColorPrev.xyz, gaussBlurColor.xyz, (1.0f - tempPreviousDepthWeight.y) * ( 1.0f - tempPreviousDepthWeight.y)), 1.0f);
				depthWeight = vec2(tempPreviousDepthWeight.x + 10.0f, tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth
//			}
		}
		
				if (tempPreviousDepthWeight.y < decayThreshold) // we are background, and below decay threshold
			{
				if (meanDepth > depthThresh){
					color = vec4(0.0f, 0.0f, 0.0f, 1.0f); // reset
					depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
				}
				else{
					color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f); // reset
					depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
				}

			}
			
	}
	
	
	
	}
	
	if (effectType == 1)
	{
	
		if (tempPreviousDepthWeight.y == 1.0f)
		{
		if (meanDepth > depthThresh)
		{
			color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
			depthWeight = vec2(meanDepth, tempPreviousDepthWeight.y - decayValue); // i.e. max weigth

		}		
		else
		{
			color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
			depthWeight = vec2(meanDepth, tempPreviousDepthWeight.y - decayValue); // i.e. max weigth
		}
				

	
		}
		else
		{
		if (meanDepth < previousMeanDepth + 10.0f) // if closer to the camera
		{
			color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
			depthWeight = vec2(previousMeanDepth + 5.0f, 1.0f); // i.e. max weigth
		}
		else
		{
			if (tempPreviousDepthWeight.y < decayThreshold) // we are background, and below decay threshold
			{
				if (meanDepth > depthThresh){
					color = vec4(0.0f, 0.0f, 0.0f, 1.0f); // reset
					depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
				}
				else{
					color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f); // reset
					depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
				}

			}
			else
			{
				color = vec4(mix(gaussBlurColorPrev.xyz, gaussBlurColor.xyz, 1.0f - tempPreviousDepthWeight.y), 1.0f);

				depthWeight = vec2(previousMeanDepth + (decayValue * 10.0f), tempPreviousDepthWeight.y - (decayValue)); // i.e. max weigth

			}
		}
		}
	
	
	
	
	}
	
		if (effectType == 2)
	{
		// if depth > depth thresh, display black
		// if depth < depth thresh 
		//// if integration weight == 1, display the current color
		//// else integration weight < 1, display the previous colour
		
		
		vec4 tempFlow = texture(_currentTextureFlow, TexCoord);
		//vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x/1.0f), TexCoord.y + (tempFlow.y/1.0f)));

		//vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x/60.0f), TexCoord.y + (tempFlow.y/33.5f)));
		vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x / 1920.0f), TexCoord.y + (tempFlow.y / 1080.0f)));

		float mag = sqrt((tempFlow.x * tempFlow.x) + (tempFlow.y * tempFlow.y));
		

		if (tempPreviousDepthWeight.y == 1.0f){
			if (sampleTex[4].x > depthThresh){
							color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);

//				color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
				depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
			}		
			else if (sampleTex[4].x < previousSampleTex[4].x + 50.0f){ // if closer to the camera
				color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
				depthWeight = vec2(tempPreviousDepthWeight.x + 10.0f, tempPreviousDepthWeight.y - 0.001f); // i.e. max weight
			}
			else if (sampleTex[4].x < depthThresh){
				color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
				depthWeight = vec2(sampleTex[4].x, 1.0f); // i.e. max weigth
			}

		}
		
		if (tempPreviousDepthWeight.y < 1.0f){
			if (sampleTex[4].x < previousSampleTex[4].x + 50.0f){ // if closer to the camera
				color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
				depthWeight = vec2(tempPreviousDepthWeight.x + 10.0f, 1.0f - 0.001f); // i.e. max weight
			}
			else{
			if (tempPreviousDepthWeight.y == 0.999f){
					//color = vec4(mix(offsetColor.xyz, tempColor.xyz, 0.01),1.0f);
					color = offsetColor;
					depthWeight = vec2(tempPreviousDepthWeight.x + 10.0f, tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth
			}
			else{
				color = vec4(samplePrev[12].xyz, 1.0f);
				depthWeight = vec2(tempPreviousDepthWeight.x + 10.0f, tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth

			}
		}
		
		
		
			//color = vec4(samplePrev[12].xyz, 1.0f);
			//depthWeight = vec2(sampleTex[4].x, tempPreviousDepthWeight.y - 0.001f); // i.e. max weigth

		}
		
		if (tempPreviousDepthWeight.y < decayThreshold){
			color = vec4(tempColor.xyz, 1.0f);
			depthWeight = vec2(sampleTex[4].x, 1.0f); // i.e. max weigth

		}
		

		
		//else{
		//	color = vec4(gaussBlurColorPrev.xyz, 1.0f);
		//	depthWeight = vec2(sampleTex[4].x, tempPreviousDepthWeight.y - 0.0001f); // i.e. max weigth

		//}
		
		
			// we have detected some kind of flow
	// we need to see if it corresponds to interesting (i.e. near field pixels)
	// if the current depth is closer to the camera than the previous depth, then its an interesting pixel
	

		
		
	


	



	
	
	
	

		
		
		

		// the flow is a 2 float x , y 
	//color = vec4(mix(offsetColor.xyz, tempColor.xyz, 0.1),1.0f);
	//color = vec4( sqrt((tempFlow.x * tempFlow.x) + (tempFlow.y * tempFlow.y)) / 10.0f , 0.0f, 0.0f, 1.0f);
	//depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth

	
	
	
	
	
	
	}
	
	if (effectType == 3)
	{
		// if depth > depth thresh, display black
		// if depth < depth thresh 
		//// if integration weight == 1, display the current color
		//// else integration weight < 1, display the previous colour
		
		
		vec4 tempFlow = texture(_currentTextureFlow, TexCoord);
		vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x/1.0f), TexCoord.y + (tempFlow.y/1.0f)));

		//vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x/60.0f), TexCoord.y + (tempFlow.y/33.5f)));
		//vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x / 1920.0f), TexCoord.y + (tempFlow.y / 1080.0f)));


		// the flow is a 2 float x , y 
	color = vec4(mix(offsetColor.xyz, tempColor.xyz, 0.1),1.0f);
	depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth

	
	
	
	
	
	
	}
	
	if (effectType == 4)
	{

		
		if(meanDepth <= previousMeanDepth + 20.0f) // if the pixel is closer to the camera, z is negative
		{
			color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
			depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
		}
		else // we are behind a previous sweep
		{
			if(tempPreviousDepthWeight.y <= 0.8) // less than threshold, so reset
			{
				color = vec4(tempColor.x, tempColor.y, tempColor.z, 1.0f);
				depthWeight = vec2(meanDepth, 1.0f); // i.e. max weigth
			}
			else if (tempPreviousDepthWeight.y <= 0.83)// 
			{
				color = vec4(mix(gaussBlurColorPrev.xyz, tempColor.xyz, 1.0f - (tempPreviousDepthWeight.y*tempPreviousDepthWeight.y)), 1.0f);
				depthWeight = vec2(previousMeanDepth + 0.1f, tempPreviousDepthWeight.y - 0.01f);
			}
			else{
				vec4 tempFlow = texture(_currentTextureFlow, TexCoord);
				vec4 offsetColor = texture(_previousTextureColor, vec2(TexCoord.x + (tempFlow.x/1920.0f), TexCoord.y + (tempFlow.y/1080.0f)));
		
				color = vec4(mix(offsetColor.xyz, gaussBlurColorPrev.xyz, 1.0f - (tempPreviousDepthWeight.y*tempPreviousDepthWeight.y)), 1.0f);
				depthWeight = vec2(previousMeanDepth + 1.1f, tempPreviousDepthWeight.y - 0.001f);

			}
			
		}

	
	}
	
	

	}
	else
	{
	vec4 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec4(texture(_currentTextureDepth, TexCoord.st + offsetsFS[i]));
    }
	
	float depthSum = 0.0f;

	for(int i = 0; i < 9; i++)
	{
		//if(sampleTex[i].x >= 2000.0f)
		//{
		//	sampleTex[i].x = 2000.0f;
		//}
			depthSum += sampleTex[i].x;
	}
	
	float meanDepth = depthSum / 9.0f;	

	
		vec4 tempPrevColor = texture(_previousTextureColor, TexCoord);
		//vec4 tempDepth = texture(_currentTextureDepth, TexCoord);
		vec4 tempPrevDepthWeight = texture(_previousTextureDepthWeight, TexCoord);

			vec4 previousSampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        previousSampleTex[i] = vec4(texture(_previousTextureDepthWeight, TexCoord));
    }
	
	float previousDepthSum = 0.0f;
	float numberOfValidPixels = 0.0f;
	for(int i = 0; i < 9; i++)
	{
		if(previousSampleTex[i].x <= 2000.0f)
		{
			previousDepthSum += previousSampleTex[i].x;
			numberOfValidPixels++;

		}
	}

		
	float previousMeanDepth = previousDepthSum / numberOfValidPixels++;
	

		if (TexCoord.x < 0.5f)
		{
			color = vec4(1.0f - meanDepth/1000.0f, 1.0f - meanDepth/1000.0f, 1.0f - meanDepth/1000.0f, 1.0f);

		}
		else
		{
			//color = vec4(1.0f - previousMeanDepth/1000.0f, 1.0f - previousMeanDepth/1000.0f, 1.0f - previousMeanDepth/1000.0f, 1.0f);
			color = vec4(1.0f - tempPrevDepthWeight.x/1000.0f, 1.0f - tempPrevDepthWeight.x/1000.0f, 1.0f - tempPrevDepthWeight.x/1000.0f, 1.0f);

		}
		
		depthWeight = vec2(meanDepth, 1.0f);

if (tempPrevDepthWeight.y < decayThreshold)
{
	depthWeight = vec2(meanDepth, 1.0f);

}
else
{
	//depthWeight = vec2(tempPrevDepthWeight.x + (decayValue * 0.01f), tempPrevDepthWeight.y - decayValue);

}
	
	//depthWeight = vec2(meanDepth + (tempPrevDepthWeight.x * 0.0001f), 1.0f); // i.e. max weigth

	
	}
    


}