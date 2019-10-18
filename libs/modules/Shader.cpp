/*
 Copyright (C) 2019 Luis Fernando García [http://luiscript.com]
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "Shader.hpp"

void Shader::setup()
{
    gotTexture = false;
    //autoShader.load("Shaders/EmptyShader");
    
    fbo.allocate(getModuleWidth(), getModuleHeight());
    fbo.begin();
    ofClear(0, 0, 0, 0);
    fbo.end();
    
    parameterName = "";
    min = 0.0;
    max = 1.0;
    precision = 2;
	loaded = false;
    
    addOutputConnection(ConnectionType::DK_FBO);
    addInputConnection(ConnectionType::DK_FBO);
}

void Shader::update()
{
    
}

void Shader::draw()
{
	if (loaded)
	{
		ofPushStyle();
		ofEnableAlphaBlending();
		if (gotTexture)
		{
			texture->bind();
		}
		fbo.begin();
		ofClear(0, 0, 0, 0);
		autoShader.begin();
		autoShader.setUniform1f("u_time", ofGetElapsedTimef());
		autoShader.setUniform2f("u_resolution", glm::vec2(getModuleWidth(), getModuleHeight()));
		for (auto& it : floatParameters)
		{
			autoShader.setUniform1f(it.first, *it.second);
		}

		for (auto& it : intParameters)
		{
			autoShader.setUniform1i(it.first, *it.second);
		}

		if (gotTexture)
		{
			ofTexture tex = texture->getTextureReference();
			autoShader.setUniformTexture("texture1", tex, 1);
			texture->draw(0, 0);
		}
		else
		{
			ofDrawRectangle(0, 0, getModuleWidth(), getModuleHeight());
		}

		autoShader.end();
		fbo.end();
		ofDisableAlphaBlending();
		ofPopStyle();
		if (gotTexture)
		{
			texture->unbind();
		}
	}
    
}


void Shader::addModuleParameters()
{
	ofxDatGuiFolder* shaderSettings = gui->addFolder("SHADER SETTINGS");
	auto newButtton = shaderSettings->addButton("New shader");
	auto openButton = shaderSettings->addButton("Open shader");

	newButtton->onButtonEvent(this, &Shader::onShaderSettingsButtonPress);
	openButton->onButtonEvent(this, &Shader::onShaderSettingsButtonPress);

	ofxDatGuiFolder* addParameter = gui->addFolder("ADD PARAMETER");
	ofxDatGuiTextInput* parameterName = addParameter->addTextInput("Name", "parameter");
	parameterName->setTextUpperCase(false);
	parameterName->onTextInputEvent(this, &Shader::onParameterNameChange);
	ofxDatGuiTextInput* parameterMin = addParameter->addTextInput("Min value", "0");
	parameterMin->onTextInputEvent(this, &Shader::onParameterMinChange);
	ofxDatGuiTextInput* parameterMax = addParameter->addTextInput("Max value", "1");
	parameterMax->onTextInputEvent(this, &Shader::onParameterMaxChange);
	ofxDatGuiTextInput* parameterPrecision = addParameter->addTextInput("Precision", "2");
	parameterPrecision->onTextInputEvent(this, &Shader::onParameterPrecisionChange);

	ofxDatGuiButton* addParameterButton = addParameter->addButton("ADD");
	addParameterButton->setLabelAlignment(ofxDatGuiAlignment::CENTER);
	addParameterButton->onButtonEvent(this, &Shader::addParameter);

	params = gui->addFolder("PARAMETERS");
}

void Shader::onShaderSettingsButtonPress(ofxDatGuiButtonEvent e)
{
	if (e.target->getName() == "New shader")
	{
		ofFileDialogResult loadFileResult = ofSystemLoadDialog("Selecciona un directorio", true, "./");
		
		if (loadFileResult.bSuccess) 
		{
			string destFragString = loadFileResult.filePath + "/emptyShader.frag";
			string destVertString = loadFileResult.filePath + "/emptyShader.vert";
			
			const GLchar* frag = 
R"END(#version 120
#extension GL_ARB_texture_rectangle : enable

uniform float u_time;
uniform vec2 u_resolution;

void main(void)
{
	gl_FragColor = vec4(1.0);
}
)END";

			const GLchar* vert = 
R"END(void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
})END";

			ofstream dest(destFragString.c_str(), ios::binary);
			dest << frag;

			ofstream destVert(destVertString.c_str(), ios::binary);
			destVert << vert;
			
			autoShader.load(loadFileResult.filePath + "/emptyShader");
			
			loaded = true;

		}
	}
	if (e.target->getName() == "Open shader")
	{
		ofFileDialogResult loadFileResult = ofSystemLoadDialog("Open shader");

		if (loadFileResult.bSuccess)
		{
			string fileString = loadFileResult.getPath();
			string shaderName = fileString.substr(0, fileString.find("."));
			autoShader.load(shaderName);
			loaded = true;
		}
	}
}


void Shader::addParameter(ofxDatGuiButtonEvent e)
{
    if(parameterName != "")
    {
        if(precision > 0)
        {
            float * newFloatParam = new float;
            ofxDatGuiSlider * newSlider;
            newSlider = params->addSlider(parameterName, min, max);
            newSlider->setPrecision(precision)->bind(*newFloatParam);
            if(getModuleMidiMapMode() != newSlider->getMidiMode())
            {
                newSlider->toggleMidiMode();
            }
            bind(*newFloatParam);
            floatParameters.insert({parameterName, newFloatParam});
        } else {
            int * newIntParam = new int;
            params->addSlider(parameterName, (int) min, (int)max, (int)min)->bind(*newIntParam);
            intParameters.insert({parameterName, newIntParam});
        }
		gui->setWidth(moduleGuiWidth);
    }
}

void Shader::onParameterNameChange(ofxDatGuiTextInputEvent e)
{
    parameterName = e.target->getText();
}

void Shader::onParameterMinChange(ofxDatGuiTextInputEvent e)
{
    min = ofToFloat(e.target->getText());
}

void Shader::onParameterMaxChange(ofxDatGuiTextInputEvent e)
{
    max = ofToFloat(e.target->getText());
}

void Shader::onParameterPrecisionChange(ofxDatGuiTextInputEvent e)
{
    precision = ofToInt(e.target->getText());
}

ofFbo * Shader::getFbo()
{
    return &fbo;
}

void Shader::setFbo(ofFbo * inFbo)
{
    texture = inFbo;
    gotTexture = inFbo != nullptr;
}
