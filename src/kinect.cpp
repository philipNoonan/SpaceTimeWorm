#include <iostream>
#include "interface.h"
#include "timing_utils.h"

#include "opencv2/optflow.hpp"
#include <opencv2/tracking.hpp>
#include "opencv2/core/utility.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#define GLUT_NO_LIB_PRAGMA
//##### OpenGL ######
#include <GL/glew.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
//#include <GL/freeglut.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/transform.hpp>

#include "shader.hpp"
#include <vector>
#include <algorithm>
#include <deque>

/////////////////////////////////////////////
// openCV STUFF
/////////////////////////////////////////////
cv::Ptr<cv::DenseOpticalFlow> algorithm;
cv::Mat m_colorImage;
cv::Mat bigFlow;
cv::Mat grey0;
cv::Mat grey1;

cv::Mat m_flow;
cv::Mat m_flow_uv[2];
cv::Mat mag, magThresh, ang;
cv::Mat hsv_split[3], hsv;
cv::Mat rgb;






//decay stuff
float decayThresholdFloat = 0.9f;
float decayValueFloat = 0.00001f;
float depthThresholdFloat = 3000.0f;
GLuint depthThresholdID;

int effectTypeInt = 0;
GLuint effectTypeID;

int screenWidth = 1920;
int screenHeight = 1080;

float *mainColor[1920 * 1080];

float colorArray[1920 * 1080];
float bigDepthArray[1920 * 1082]; // 1082 is not a typo
float color[512 * 424];
float depth[512 * 424];
float infra[512 * 424];
bool useLowResolution = false;

GLFWwindow *win;
GLuint textureColor;
GLuint textureDepth;
GLuint textureFlow;
GLuint programID;
GLuint programID_screen;
GLuint VBO, VAO, EBO;
GLuint framebuffer;
GLuint textureColorbuffer;
GLuint previousTextureColor;
GLuint currentTextureDepthWeight, previousTextureDepthWeight;
GLuint currentTextureFlow;

GLuint quadVAO, quadVBO, quadEBO;

// Set up texture sampling offset storage
const GLint tcOffsetColumns = 5;
const GLint tcOffsetRows = 5;
GLint filterNumberCode = 0;
GLfloat	texCoordOffsets[tcOffsetColumns * tcOffsetRows * 2];

GLuint renderColorID;
GLuint tcOffsetID, filterNumberID;
GLuint tcOffsetID_screen;
GLuint decayValueID, decayThresholdID;
GLfloat offsetStep = 1.0f;
GLint renderColor = 1;
//cv::Mat color;
//cv::Mat depth;
//cv::Mat infra;

GLuint pboIds[2];
GLuint pboIdsDepth[2];

// Calculate texture coordinate offsets for kernel convolution effects
void genTexCoordOffsets(GLuint width, GLuint height, GLfloat step = 1.0f) // Note: Change this step value to increase the number of pixels we sample across...
{
	// Note: You can multiply the step to displace the samples further. Do this with diff values horiz and vert and you have directional blur of a sort...
	float xInc = step / (GLfloat)(width);
	float yInc = step / (GLfloat)(height);

	for (int i = 0; i < tcOffsetColumns; i++)
	{
		for (int j = 0; j < tcOffsetRows; j++)
		{
			texCoordOffsets[(((i * 5) + j) * 2) + 0] = (-2.0f * xInc) + ((GLfloat)i * xInc);
			texCoordOffsets[(((i * 5) + j) * 2) + 1] = (-2.0f * yInc) + ((GLfloat)j * yInc);
		}
	}
}

// Generates a texture that is suited for attachments to a framebuffer
GLuint generateAttachmentTexture(GLboolean depth, GLboolean stencil)
{
	// What enum to use?
	GLenum attachment_type;
	if (!depth && !stencil)
		attachment_type = GL_RGB;
	else if (depth && !stencil)
		attachment_type = GL_DEPTH_COMPONENT;
	else if (!depth && stencil)
		attachment_type = GL_STENCIL_INDEX;

	//Generate texture ID and load texture data 
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	if (!depth && !stencil)
		glTexImage2D(GL_TEXTURE_2D, 0, attachment_type, screenWidth, screenHeight, 0, attachment_type, GL_UNSIGNED_BYTE, NULL);
	else // Using both a stencil and depth test, needs special format arguments
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, screenWidth, screenHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	return textureID;
}


void renderToFramebuffer()
{
	genTexCoordOffsets(1920, 1080, offsetStep);


	static int index = 0;
	int nextIndex = 0;                  // pbo index used for next frame

	// In dual PBO mode, increment current index first then get the next index
	index = (index + 1) % 2;
	nextIndex = (index + 1) % 2;

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	//glEnable(GL_DEPTH_TEST);
	glUseProgram(programID);

	int w, h;
	glfwGetFramebufferSize(win, &w, &h);

	glViewport(0, 0, w, h);

	glUniform1f(decayValueID, decayValueFloat);
	glUniform1f(decayThresholdID, decayThresholdFloat);
	glUniform1i(renderColorID, renderColor);
	glUniform2fv(tcOffsetID, 25, texCoordOffsets);
	glUniform1f(depthThresholdID, depthThresholdFloat);
	glUniform1i(effectTypeID, effectTypeInt);

	//std::cout << "value: " << decayValueFloat << " Threshold: " << decayThresholdFloat << std::endl;


	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // dont clear otherwise effect is lost?
	//glClear(GL_COLOR_BUFFER_BIT);

	//std::thread colorUpload(colorUploadThread);
	//std::thread depthUpload(depthUploadThread);

	// bind the texture and PBO
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, textureColor);
	//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[index]);
	//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1920, 1080, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[nextIndex]);
	//glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 1920*1080*4, 0, GL_STREAM_DRAW_ARB);
	//GLubyte* ptr = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	//memcpy_s(ptr, 1920 * 1080 * 4, colorArray, 1920 * 1080 * 4);
	//glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
	//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);


	//// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColor);
	////glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1920, 1080, 0, GL_BGRA, GL_UNSIGNED_BYTE, colorArray);
	glTexSubImage2D(GL_TEXTURE_2D, /*level*/0, /*xoffset*/0, /*yoffset*/0, 1920, 1080, GL_BGRA, GL_UNSIGNED_BYTE, colorArray);
	glUniform1i(glGetUniformLocation(programID, "_currentTextureColor"), 0);

	//// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textureDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 1920, 1082, 0, GL_RED, GL_FLOAT, bigDepthArray);
	////glTexSubImage2D(GL_TEXTURE_2D, /*level*/0, /*xoffset*/0, /*yoffset*/0, 1920, 1082, GL_R32F, GL_FLOAT, bigDepthArray);
	glUniform1i(glGetUniformLocation(programID, "_currentTextureDepth"), 1);

	//// Bind Textures using texture units
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textureFlow);
	cv::resize(m_flow, bigFlow, cv::Size(), 4, 4, cv::INTER_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 1920, 1080, 0, GL_RG, GL_FLOAT, bigFlow.ptr());
	////glTexSubImage2D(GL_TEXTURE_2D, /*level*/0, /*xoffset*/0, /*yoffset*/0, 1920, 1082, GL_R32F, GL_FLOAT, bigDepthArray);
	glUniform1i(glGetUniformLocation(programID, "_currentTextureFlow"), 2);


	// bind the texture and PBO
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, textureDepth);
	//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsDepth[index]);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 1920, 1082, 0, GL_RED, GL_FLOAT, 0);
	////glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1920, 1082, GL_RED, GL_FLOAT, 0);
	//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsDepth[nextIndex]);
	//glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 1920 * 1082 * 4, 0, GL_STREAM_DRAW_ARB);
	//GLubyte* ptrDepth = (GLubyte*)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	//memcpy_s(ptrDepth, 1920 * 1082 * 4, bigDepthArray, 1920 * 1082 * 4);
	//glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release pointer to mapping buffer
	//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);





	//colorUpload.join();
	//depthUpload.join();


	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, previousTextureColor);
	glUniform1i(glGetUniformLocation(programID, "_previousTextureColor"), 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, previousTextureDepthWeight);
	glUniform1i(glGetUniformLocation(programID, "_previousTextureDepthWeight"), 4);


	// Draw container
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	//glActiveTexture(0);



}

void renderToScreen()
{

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//int w, h;
	//glfwGetFramebufferSize(win, &w, &h);

	//glViewport(0, 0, w, h);



	//genTexCoordOffsets(1920, 1080, offsetStep);

	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
	//glClear(GL_COLOR_BUFFER_BIT);
	//glDisable(GL_DEPTH_TEST); // We don't care about depth information when rendering a single quad
	glUseProgram(programID_screen);	



	glUniform1i(filterNumberID, filterNumberCode);
	glUniform2fv(tcOffsetID_screen, 25, texCoordOffsets);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// Use the color attachment texture as the texture of the quad plane


	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//glBindVertexArray(VAO);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	//glBindVertexArray(0);


}

void copyToPreviousFrame()
{
	glCopyImageSubData(currentTextureDepthWeight, GL_TEXTURE_2D, 0, 0, 0, 0,
						previousTextureDepthWeight, GL_TEXTURE_2D, 0, 0, 0, 0,
					   screenWidth, screenHeight, 1);

	glCopyImageSubData(textureColorbuffer, GL_TEXTURE_2D, 0, 0, 0, 0,
					previousTextureColor, GL_TEXTURE_2D, 0, 0, 0, 0,
					screenWidth, screenHeight, 1);

	//GLuint tc = previousTextureColor;
	//previousTextureColor = textureColor;
	//textureColor = tc;

	//GLuint td = previousTextureDepthWeight;
	//previousTextureDepthWeight = currentTextureDepthWeight;
	//currentTextureDepthWeight = td;


}


void display()
{

	renderToFramebuffer();

	renderToScreen();

	copyToPreviousFrame();
	
	




}


void createFBO()
{
// The offscreen framebuffer to store history of frames for temporal blurring
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	// Create a color attachment texture
	textureColorbuffer = generateAttachmentTexture(false, false);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, currentTextureDepthWeight, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, currentTextureFlow, 0);

	GLenum buffers_to_render[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 }; // this allows glsl to write to 2 outputs on the layout thingy
	glDrawBuffers(3, buffers_to_render);

	// Create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight); // Use a single renderbuffer object for both a depth AND stencil buffer.
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // Now actually attach it
	// Now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	glDeleteFramebuffers(1, &framebuffer);


	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, currentTextureDepthWeight); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, previousTextureDepthWeight); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);

	createFBO();


}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
		filterNumberCode = 0;
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		filterNumberCode = 1;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		filterNumberCode = 2;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		filterNumberCode = 3;
	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
		filterNumberCode = 4;
	if (key == GLFW_KEY_5 && action == GLFW_PRESS)
		filterNumberCode = 5;
	if (key == GLFW_KEY_6 && action == GLFW_PRESS)
		filterNumberCode = 6;
	if (key == GLFW_KEY_7 && action == GLFW_PRESS)
		filterNumberCode = 7;
	if (key == GLFW_KEY_8 && action == GLFW_PRESS)
		filterNumberCode = 8;
	if (key == GLFW_KEY_9 && action == GLFW_PRESS)
		filterNumberCode = 9;

	if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS)
		offsetStep += 0.1f;
	if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS)
		offsetStep -= 0.1f;


	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		renderColor = !renderColor;

	// decay stuff
	if (key == GLFW_KEY_PERIOD)
		depthThresholdFloat += 100.0f;
	if (key == GLFW_KEY_COMMA)
		depthThresholdFloat -= 100.0f;

	if (key == GLFW_KEY_E  && action == GLFW_PRESS)
	{
		effectTypeInt++;
		if (effectTypeInt > 4){
			effectTypeInt = 0;
		}

		if (effectTypeInt == 0)
		{
			decayValueFloat = 0.00001f;
			decayThresholdFloat = 0.9f;
		}
		else if (effectTypeInt == 1)
		{
			decayValueFloat = 0.0044f;
			decayThresholdFloat = 0.91f;
		}
		else if (effectTypeInt == 2)
		{

		}
		else if (effectTypeInt == 3)
		{

		}



	}
	 

	// depth thresh stuff
	if (key == GLFW_KEY_UP && action == GLFW_PRESS)
		decayValueFloat += 0.0005f;
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
		decayValueFloat -= 0.0005f;


	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		decayThresholdFloat += 0.01f;
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		decayThresholdFloat -= 0.01f;

	std::cout << "effectType: " << effectTypeInt << " value: " << decayValueFloat << " Threshold: " << decayThresholdFloat << " Depth Thresh: " << depthThresholdFloat << std::endl;

}

void doOpticalFlow()
{
	if (grey1.empty())
	{
		grey0.copyTo(grey1);
	}

	//cv::Rect ROI(0, 0, 1920/4, 1080/2);
	//grey0(ROI).setTo(cv::Scalar::all(0));


	//cvtColor(col0, grey0, cv::COLOR_BGRA2GRAY);
	//cvtColor(col1, grey1, cv::COLOR_BGRA2GRAY);

	cv::Mat flow;
	algorithm->calc(grey0, grey1, flow);
	//algorithm->collectGarbage();
	
	
	flow.copyTo(m_flow);
	cv::split(flow, m_flow_uv);
	cv::multiply(m_flow_uv[1], -1, m_flow_uv[1]);
	cv::cartToPolar(m_flow_uv[0], m_flow_uv[1], mag, ang, true);
	

	cv::threshold(mag, magThresh, 0, 255, CV_THRESH_TOZERO);
	cv::normalize(magThresh, magThresh, 0, 1, cv::NORM_MINMAX);

	hsv_split[0] = ang;
	hsv_split[1] = magThresh;
	hsv_split[2] = cv::Mat::ones(ang.size(), ang.type());
	cv::merge(hsv_split, 3, hsv);
	cv::cvtColor(hsv, rgb, cv::COLOR_HSV2BGR);
	cv::imshow("g0", grey0);
	cv::imshow("g1", grey1);
	cv::imshow("diff", (grey0 - grey1) * 10);
	cv::imshow("flow1", rgb);

	//cv::waitKey(1);

	//Mat flow_split[2];
	//Mat magnitude, angle;
	//Mat hsv_split[3], hsv, rgb;
	//split(flow, flow_split);
	//cartToPolar(flow_split[0], flow_split[1], magnitude, angle, true);
	//normalize(magnitude, magnitude, 0, 1, NORM_MINMAX);
	//hsv_split[0] = angle; // already in degrees - no normalization needed
	//hsv_split[1] = Mat::ones(angle.size(), angle.type());
	//hsv_split[2] = magnitude;
	//merge(hsv_split, 3, hsv);
	//cvtColor(hsv, rgb, COLOR_HSV2BGR);


	std::swap(grey1, grey0);




}




int main(int argc, char* argv[])
{




#if NDEBUG
	//algorithm = createOptFlow_DIS(cv::optflow::DISOpticalFlow::PRESET_MEDIUM);
	algorithm = createOptFlow_DIS(cv::optflow::DISOpticalFlow::PRESET_MEDIUM);
	//cv::Ptr<cv::DenseOpticalFlow> algorithm;

	//cv::optflow:: dis 
	//DISOpticalFlowImpl()

	////cv::Ptr<cv::optflow::DISOpticalFlow> 
	//algorithm = cv::makePtr<cv::optflow::DISOpticalFlowImpl>();
	//algorithm->setPatchSize(8);

	//algorithm->setFinestScale(2);
	//algorithm->setPatchStride(4);
	//algorithm->setGradientDescentIterations(12);
	//algorithm->setVariationalRefinementIterations(0);




	
#else 
	algorithm = createOptFlow_DIS(cv::optflow::DISOpticalFlow::PRESET_ULTRAFAST);
#endif





















	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_REFRESH_RATE, 30);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	win = glfwCreateWindow(screenWidth, screenHeight, "SpaaaaceTimeWorm", nullptr, nullptr);
	//glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
	glfwSetKeyCallback(win, key_callback);

	if (win == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(win);

	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	programID = LoadShaders("resources/vertShader.glsl", "resources/fragShader.glsl");
	programID_screen = LoadShaders("resources/vertShader_screen.glsl", "resources/fragShader_screen.glsl");

	decayValueID = glGetUniformLocation(programID, "decayValue");
	decayThresholdID = glGetUniformLocation(programID, "decayThreshold");
	renderColorID = glGetUniformLocation(programID, "renderColor");
	tcOffsetID = glGetUniformLocation(programID, "tcOffset");
	depthThresholdID = glGetUniformLocation(programID, "depthThresh");
	effectTypeID = glGetUniformLocation(programID, "effectType");


	filterNumberID = glGetUniformLocation(programID_screen, "filterNumber");
	tcOffsetID_screen = glGetUniformLocation(programID_screen, "tcOffset");



	glGenBuffersARB(2, pboIds);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 1920 * 1080 * 4, 0, GL_STREAM_DRAW_ARB);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 1920 * 1080 * 4, 0, GL_STREAM_DRAW_ARB);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	glGenBuffersARB(2, pboIdsDepth);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsDepth[0]);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 1920 * 1082 * 4, 0, GL_STREAM_DRAW_ARB);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pboIdsDepth[1]);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 1920 * 1082 * 4, 0, GL_STREAM_DRAW_ARB);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	GLfloat vertices[] = {
		// Positions		// Texture coords
		 1.0f, 1.0f, 0.0f,		1.0f, 1.0f, // top right
		 1.0f,-1.0f, 0.0f,		1.0f, 0.0f, // bottom right
		-1.0f,-1.0f, 0.0f,		0.0f, 0.0f, // bottom left
		-1.0f, 1.0f, 0.0f,		0.0f, 1.0f  // Top left
	};

	GLuint indices[] = {  // Note that we start from 0!
		0, 1, 3, // First Triangle
		1, 2, 3  // Second Triangle
	};


	GLfloat quadVertices[] = {   // Vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// Positions   // TexCoords
		-1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,

		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);


	int width, height;
	glfwGetFramebufferSize(win, &width, &height);
	glViewport(0, 0, width, height);



	//////////////////////////////
	// CURRENT FRAME
	//////////////////////////////
	// Texture Color generate
	glGenTextures(1, &textureColor);
	glBindTexture(GL_TEXTURE_2D, textureColor); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Texture Depth Generate
	glGenTextures(1, &textureDepth);
	glBindTexture(GL_TEXTURE_2D, textureDepth); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &currentTextureDepthWeight);
	glBindTexture(GL_TEXTURE_2D, currentTextureDepthWeight); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	////////////////////////////////
	// FLOW TEXTURE
	////////////////////////////////////
	glGenTextures(1, &textureFlow);
	glBindTexture(GL_TEXTURE_2D, textureFlow); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	//////////////////////////////
	// PREVIOUS FRAME
	//////////////////////////////
	//generate previous frame texture 
	glGenTextures(1, &previousTextureColor);
	glBindTexture(GL_TEXTURE_2D, previousTextureColor); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &previousTextureDepthWeight);
	glBindTexture(GL_TEXTURE_2D, previousTextureDepthWeight); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);



	// setup screen vao
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glBindVertexArray(0);


	//////////////////////////////
	// THE FRAMEBUFFER
	//////////////////////////////
	createFBO();



	Freenect2Camera camera;

	camera.start();

	while (!camera.ready())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	FrequencyMonitor fps;

	bool stop = false;
	int frame_counter = 1;

	double lastTime = glfwGetTime();
	int nbFrames = 0;


	while (!glfwWindowShouldClose(win))
	{

		if (camera.ready())
		{
			
		if (useLowResolution)
		{
			camera.frames(color, depth, infra);
		}
		else
		{
			camera.frames(colorArray, bigDepthArray);
		}

		cv::resize(cv::Mat(1080, 1920, CV_8UC4, colorArray), m_colorImage, cv::Size(), 0.25, 0.25, cv::INTER_LINEAR);
		cv::cvtColor(m_colorImage, grey0, cv::COLOR_BGRA2GRAY);

		doOpticalFlow();


			display();

			
			glfwSwapBuffers(win);
			glfwPollEvents();

			// Measure speed
			double currentTime = glfwGetTime();
			nbFrames++;
			if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1 sec ago
				// printf and reset timer
				printf("%f ms/frame\n", 1000.0 / double(nbFrames));
				nbFrames = 0;
				lastTime += 1.0;
			}




		}




	}

	camera.stop();
	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteBuffersARB(2, pboIds);
	glfwTerminate();
	return 0;
}
