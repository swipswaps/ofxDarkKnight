/*
 Copyright (C) 2018 Luis Fernando García [http://luiscript.com]
 
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

#ifndef mixer_h
#define mixer_h

#include "DKModule.hpp"
#include "unordered_map"

class DKMixer : public DKModule
{
private:
    ofBlendMode blendMode;
    ofFbo compositionFbo;
    vector<ofFbo*> fbos;
    unordered_map<string, ofBlendMode> blendModes;
    bool drawOutput;
public:
    void setup()
    {
        blendModes.insert({"DISABLED" , OF_BLENDMODE_DISABLED});
        blendModes.insert({"ALPHA" , OF_BLENDMODE_ALPHA});
        blendModes.insert({"ADD" , OF_BLENDMODE_ADD});
        blendModes.insert({"SUBTRACT" , OF_BLENDMODE_SUBTRACT});
        blendModes.insert({"MULTIPLY" , OF_BLENDMODE_MULTIPLY});
        blendModes.insert({"SCREEN" , OF_BLENDMODE_SCREEN});
        
        drawOutput = false;
        
        compositionFbo.allocate(getModuleWidth(), getModuleHeight(), GL_RGBA, 4);
        compositionFbo.begin();
        ofClear(0,0,0,0);
        compositionFbo.end();
        
		addInputConnection(DKConnectionType::DK_FBO, "fbo1");
		addInputConnection(DKConnectionType::DK_FBO, "fbo2");
        addOutputConnection(DKConnectionType::DK_FBO);
        
        fbos.clear();
        
        gui->setWidth(250);
        blendMode = OF_BLENDMODE_DISABLED;
    }
    
    void update()
    {
		
    }
    
    void draw()
    {
        compositionFbo.begin();
        ofClear(0,0,0,0); 
        for(auto fbo : fbos)
        {
			if (fbo != nullptr)
			{
				fbo->draw(0, 0);
				ofEnableBlendMode(blendMode);
			}
        }
        ofDisableBlendMode();
        compositionFbo.end();
    }
    
    void addModuleParameters()
    {
        vector<string> blendModesLabels;
        blendModesLabels.push_back("DISABLED");
        blendModesLabels.push_back("ALPHA");
        blendModesLabels.push_back("ADD");
        blendModesLabels.push_back("SUBTRACT");
        blendModesLabels.push_back("MULTIPLY");
        blendModesLabels.push_back("SCREEN");
        
        ofxDatGuiComponent * component = gui->addDropdown("BLEND MODE", blendModesLabels);
        component->onDropdownEvent(this, &DKMixer::onBlendModeChanges);
    }
    
    ofFbo * getFbo()
    {
        return &compositionFbo;
    }
    
    void setFbo(ofFbo * fboPtr)
    {
		if (fboPtr == nullptr)
		{
			vector<ofFbo*> fbosTemp;

			for (int i = 0; i < inputs.size(); i++)
			{
				if (inputs[i]->getFbo() != nullptr)
				{
					fbosTemp.push_back(inputs[i]->getFbo());
				}
			}

			fbos.clear();

			for ( int i = 0; i < fbosTemp.size(); i++)
			{
				fbos.push_back(fbosTemp[i]);
			}
			
			fbosTemp.clear();
		}
		else 
		{
			fbos.push_back(fboPtr);
		}
    }
    
    void onBlendModeChanges(ofxDatGuiDropdownEvent e )
    {
        string mode =  e.target->getSelected()->getName();
        unordered_map<string, ofBlendMode>::iterator it;
        it = blendModes.find(mode);
        if(it != blendModes.end())
        {
            blendMode = blendModes[mode];
        }
    }

};

#endif /* mixer_h */