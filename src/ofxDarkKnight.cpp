/*
 Copyright (C) 2018 Luis Fernando Garc�a P�rez [http://luiscript.com]
 
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

#include "ofxDarkKnight.hpp"

ofxDarkKnight::ofxDarkKnight()
{
    
}

ofxDarkKnight::~ofxDarkKnight()
{
    
}

void ofxDarkKnight::setup()
{
    loadWires = shiftKey = altKey = cmdKey = midiMapMode = drawing = showExplorer = false;
    translation = { 0, 0 };
    resolution = { 1920, 1080 };
	zoom = 1.0;
	moduleId = 1;
    

    for(auto module : moduleList ) poolNames.push_back(module.first);

    poolNames.sort();
    
	float browserWidth = ofGetWidth() * 0.8;
	float browserHeight = ofGetHeight() * 0.8;
    int compHeight = poolNames.size() * 22.0;
	componentsList = new ofxDatGuiScrollView("MODULES");
	componentsList->setWidth(browserWidth / 3);
	componentsList->setHeight(compHeight);
	componentsList->setPosition(ofGetWidth()/2 - 0.5 * browserWidth / 3, ofGetHeight()/2 - compHeight/2);
	componentsList->onScrollViewEvent(this, &ofxDarkKnight::onComponentListChange);
        
    for(auto it = poolNames.begin(); it != poolNames.end(); it++) componentsList->add(*it);

    ofAddListener(ofEvents().mousePressed,  this, &ofxDarkKnight::handleMousePressed);
    ofAddListener(ofEvents().mouseDragged,  this, &ofxDarkKnight::handleMouseDragged);
    ofAddListener(ofEvents().mouseScrolled, this, &ofxDarkKnight::handleMouseScrolled);
	ofAddListener(ofEvents().mouseReleased, this, &ofxDarkKnight::handleMouseReleased);
	ofAddListener(ofEvents().keyPressed,    this, &ofxDarkKnight::handleKeyPressed);
	ofAddListener(ofEvents().keyReleased,   this, &ofxDarkKnight::handleKeyReleased);
	ofAddListener(ofEvents().fileDragEvent, this, &ofxDarkKnight::handleDragEvent);
	
	#ifdef TARGET_OSX
		darkKnightMidiOut.openVirtualPort("ofxDarkKnight");
	#else
	if (darkKnightMidiOut.getNumOutPorts() > 0)
	{
		darkKnightMidiOut.openPort(darkKnightMidiOut.getNumOutPorts()-1);
	}
	#endif
	addModule("PROJECT");

}
 

void ofxDarkKnight::update()
{
    for (auto wire : wires)
        if(wire.inputModule->getModuleEnabled() &&
           wire.outputModule->getModuleEnabled())
            wire.update();
    
    for(pair<string, DKModule*> module : modules )
        if(module.second->getModuleEnabled())
        {
            module.second->updateModule(translation.x, translation.y, zoom);
            for(auto msg : module.second->outMidiMessages)
            {
                sendMidiMessage(*msg);
            }
            module.second->outMidiMessages.clear();
        }
    
	if (showExplorer) {
		componentsList->update();
	}
}


void ofxDarkKnight::draw()
{
    ofPushMatrix();
    ofTranslate(translation.x, translation.y);
	ofScale(zoom);
    
    if(drawing) currentWire->drawCurrentWire(pointer);
    
    for(auto wire : wires) wire.draw();
    
    for(auto module : modules )
        if(!module.second->moduleIsChild && module.second->getModuleEnabled())
            module.second->drawModule();

    ofPopMatrix();
    
	componentsList->setVisible(showExplorer);
    if(showExplorer) componentsList->draw();

}

void ofxDarkKnight::toggleList()
{
    showExplorer = !showExplorer;
}

void ofxDarkKnight::toggleMappingMode()
{
    midiMapMode = !midiMapMode;
    
    for(pair<string, DKModule*> module : modules )
    {
        module.second->toggleMidiMap();
		if (module.second->getModuleHasChild())
		{
			DKMediaPool* mp = static_cast<DKMediaPool*>(module.second);
			mp->drawMediaPool();
		}
    }
}

void ofxDarkKnight::onComponentListChange(ofxDatGuiScrollViewEvent e)
{
    addModule(e.target->getName());
    toggleList();
}

void ofxDarkKnight::handleKeyPressed(ofKeyEventArgs& keyboard)
{
	if (keyboard.key == OF_KEY_COMMAND || keyboard.key == OF_KEY_CONTROL) cmdKey = true;
	if (keyboard.key == OF_KEY_SHIFT) shiftKey = true;
	if (keyboard.key == OF_KEY_ALT) altKey = true;

	// toggle show explorer
	if (cmdKey && keyboard.keycode == 257 && !keyboard.isRepeat)
	{
		toggleList();
	}

	// cmd+shift+m  -> Toggle midiMap on all layers
	if (cmdKey && keyboard.keycode == 77 && !keyboard.isRepeat) {
		toggleMappingMode();
	}

	if (cmdKey && keyboard.key == OF_KEY_BACKSPACE)
	{
		deleteFocusedModule();
	}

	//cmd + shift + 's' to save preset
	if (shiftKey && cmdKey && keyboard.key == 's')
	{
		savePreset();
	}

	//cmd + r reset translation and zoom
	if (cmdKey && keyboard.keycode == 82)
	{
		translation = { 0, 0 };
		zoom = 1.0;
	}
}

void ofxDarkKnight::handleKeyReleased(ofKeyEventArgs& keyboard)
{
	if (keyboard.key == OF_KEY_COMMAND || keyboard.key == OF_KEY_CONTROL) cmdKey = false;
	if (keyboard.key == OF_KEY_SHIFT) shiftKey = false;
	if (keyboard.key == OF_KEY_ALT) altKey = false;
}

void ofxDarkKnight::handleMousePressed(ofMouseEventArgs &mouse)
{
	int x = (int)(mouse.x - translation.x) / zoom;
	int y = (int)(mouse.y - translation.y) / zoom;

	checkOutputConnection(x, y, "*");

	if (shiftKey)
	{
		#ifdef _WIN32
		SetCursor(LoadCursor(NULL, IDC_SIZEALL));
		#endif
		startX = mouse.x;
		startY = mouse.y;
	}
	
    for(pair<string, DKModule*> module : modules )
    {
        //send mouse arguments to modules with childs (like Media Pool)
        if(module.second->getModuleHasChild())
        {
            DKMediaPool * mp = static_cast<DKMediaPool*>(module.second);
            mp->mousePressed(mouse);
        }
    }
    
	//right click?
	if (mouse.button == 2)
	{
		toggleList();
	}

	//middle click
	if (mouse.button == 1)
	{
		toggleMappingMode();
	}

}

void ofxDarkKnight::handleMouseDragged(ofMouseEventArgs & mouse)
{
	if (shiftKey)
	{
		#ifdef _WIN32
		SetCursor(LoadCursor(NULL, IDC_SIZEALL));
		#endif
		translation.x += mouse.x - startX;
		translation.y += mouse.y - startY;

		startX = mouse.x;
		startY = mouse.y;
	}

    if(drawing)
    {
        pointer.x = (mouse.x - translation.x) / zoom;
        pointer.y = (mouse.y - translation.y) / zoom;
    }
}

void ofxDarkKnight::handleMouseReleased(ofMouseEventArgs & mouse)
{
    int x = (int) (mouse.x - translation.x) / zoom;
    int y = (int) (mouse.y - translation.y) / zoom;

        checkInputConnection(x, y, "*");
}

void ofxDarkKnight::handleMouseScrolled(ofMouseEventArgs & mouse)
{
    //translation is not fully supported with ofxDarkKnightMapping, be careful.
	if (shiftKey)
	{
		if (zoom >= 0.15)
		{
			zoom += mouse.scrollY * 0.05;
		}
		else
		{
			zoom = 0.15;
		}
	}
}

void ofxDarkKnight::checkOutputConnection(float x, float y, string moduleName)
{
    DKWireConnection * output;
    DKWireConnection * input;
    
    for(pair<string, DKModule*> module : modules )
    {
        if(!module.second->getModuleEnabled()) continue;
        output = module.second->getOutputConnection(x, y);
        
        //true if we click on output connection
        if(output != nullptr && module.second->getModuleEnabled() &&
           (module.second->getName() == moduleName || moduleName == "*" ))
        {
			currentWireConnectionType = output->getConnectionType();
            currentWire = new DKWire;
            currentWire->setOutputConnection(output);
            currentWire->setConnectionType(currentWireConnectionType);
            currentWire->outputModule = module.second;
            if(currentWireConnectionType == DKConnectionType::DK_FBO)
            {
				output->setFbo(module.second->getFbo());
                currentWire->fbo = module.second->getFbo();
			}
			else if (currentWireConnectionType == DKConnectionType::DK_LIGHT)
			{
				output->setLight(module.second->getLight());
				currentWire->light = module.second->getLight();
			}

            pointer.x = x;
            pointer.y = y;
            drawing = true;
            break;
        }
        
        input = module.second->getInputConnection(x, y);
        //true if we click on input node
        if(input != nullptr)
        {
            //loop all the wires to find an input match
            for(auto it = wires.begin(); it != wires.end(); it++)
            {
                //true if the input clicked is on the list (existing cable)
                // we need to disconect the wire and delete it from the list
                if(it->getInput() == input)
                {
                    currentWire = new DKWire;
                    currentWire->setConnectionType(input->getConnectionType());
                    currentWire->setOutputConnection(it->getOutput());
                    currentWire->outputModule = it->outputModule;
                    
                    if (input->getConnectionType() == DKConnectionType::DK_MULTI_FBO)
                    {
                        int connectionIndex(input->getIndex());
                        input->setFbo(nullptr);
                        it->inputModule->setFbo(nullptr, connectionIndex);
                        currentWire->fbo = it->outputModule->getFbo();
                    }
                    else if (currentWire->getConnectionType() == DKConnectionType::DK_FBO)
                    {
						input->setFbo(nullptr);
                        it->inputModule->setFbo(nullptr);
                        currentWire->fbo = it->outputModule->getFbo();
					}
					else if (currentWire->getConnectionType() == DKConnectionType::DK_LIGHT)
					{
						input->setLight(nullptr);
						it->inputModule->setLight(nullptr);
						currentWire->light = it->output->getLight();
					}
                    else if (currentWire->getConnectionType() == DKConnectionType::DK_CHAIN)
                    {
                        currentWire->outputModule->setChainModule(nullptr);
                    }
                    
                    pointer.x = x;
                    pointer.y = y;
                    
                    wires.erase(it);
                    drawing = true;
                    break;
                }
            }
        }
    }
}


void ofxDarkKnight::checkInputConnection(float x, float y, string moduleName)
{
    DKWireConnection * input;
    
    for(pair<string, DKModule*> module : modules )
    {
        if(!module.second->getModuleEnabled()) continue;
        input = module.second->getInputConnection(x, y);
        //true if user released the wire on input connection
        if(input != nullptr && currentWire != nullptr &&
           (module.second->getName() == moduleName || moduleName == "*"))
        {
            currentWire->setInputConnection(input);
            currentWire->setInputModule(module.second);
            
            //les check if the input connection is a multi fbo
            if(input->getConnectionType() == DKConnectionType::DK_MULTI_FBO &&
               currentWire->getOutput()->getConnectionType() == DKConnectionType::DK_FBO)
            {
                int connectionIndex(input->getIndex());
                auto inputCon = module.second->getInputConnection(x, y) ;
                inputCon->setFbo(currentWire->fbo);
                inputCon->setIndex((unsigned) input->getIndex());
                module.second->setFbo(currentWire->fbo, connectionIndex);
            }
            //if current dragging wire connection is different from the input break the search
            else if(input->getConnectionType() != currentWire->getOutput()->getConnectionType())
            {
                currentWire = nullptr;
                drawing = false;
                break;
            }
            
            if (currentWire->getConnectionType() == DKConnectionType::DK_SLIDER)
            {
                currentWire->slider = static_cast<ofxDatGuiSlider*>(module.second->getInputComponent(x, y));
            }
            else if(currentWire->getConnectionType() == DKConnectionType::DK_FBO)
            {
				module.second->getInputConnection(x, y)->setFbo(currentWire->fbo);
                module.second->setFbo(currentWire->fbo);
			}
			else if (currentWire->getConnectionType() == DKConnectionType::DK_LIGHT)
			{
				module.second->getInputConnection(x, y)->setLight(currentWire->light);
				module.second->setLight(currentWire->light);
			}
            else if (currentWire->getConnectionType() == DKConnectionType::DK_CHAIN)
            {
                currentWire->outputModule->setChainModule(module.second);
            }
            
            wires.push_back(*currentWire);
            drawing = false;
            currentWire = nullptr;
            break;
        }
    }
    
    currentWire = nullptr;
    drawing = false;
}

void ofxDarkKnight::handleDragEvent(ofDragInfo & dragInfo)
{
    bool mediaPoolFounded = false;
   /* if(dragInfo.files.size() > 0)
    {
        ofFile file(dragInfo.files[0]);
        if(file.exists())
        {
            string fileExtension = ofToUpper(file.getExtension());
            if(fileExtension == "MOV" || fileExtension == "MP4")
            {
                DKHap * hapPlayer = new DKHap;
                
                for(pair<string, DKModule*> module : modules )
                    //this is true only for Media Pool modules
                    if(module.second->getModuleHasChild())
                    {
                        DKMediaPool * mp = static_cast<DKMediaPool*>(module.second);
                        float amp = 1.0;
                        mp->addItem(hapPlayer, "thumbnails/terrain.jpg", "video player");
                        mediaPoolFounded = true;
                        hapPlayer->loadFile(file.getAbsolutePath());
                        hapPlayer->gui->setWidth(mp->gui->getWidth());
                        modules.insert({"HAP: " + file.getFileName(), hapPlayer});
                        
                        mp->drawMediaPool();
                        return;
                    }
                
                if(!mediaPoolFounded)
                {
                    DKMediaPool * newPool = new DKMediaPool;
                    
                    newPool->setCollectionName("Collection 1");
                    newPool->setupModule("SKETCH POOL 1", resolution, false);
                    newPool->gui->setPosition(ofGetMouseX() + 15, ofGetMouseY() + 15);
                    newPool->setModulesReference(&modules);
                    newPool->setTranslationReferences(&translation, &zoom);
                    newPool->setModuleMidiMapMode(midiMapMode);
                    newPool->addItem(hapPlayer, "thumbnails/terrain.jpg", "HAP: " + file.getFileName());
                    
                    hapPlayer->gui->setWidth(newPool->gui->getWidth());
                    hapPlayer->loadFile(file.getAbsolutePath());
                    hapPlayer->setModuleMidiMapMode(midiMapMode);
                    
                    modules.insert({"HAP: " + file.getFileName(), hapPlayer});
                    modules.insert({"SKETCH POOL 1", newPool});
                   
                    return;
                }
            }
        }
    }*/
}


void ofxDarkKnight::addModule(string moduleName, DKModule * module)
{
    module->setModuleMidiMapMode(midiMapMode);
	module->setModuleId(getNextModuleId());
    modules.insert({moduleName, module});
}

DKModule * ofxDarkKnight::addModule(string moduleName)
{
	int moduleId = getNextModuleId();
	string uniqueModuleName = moduleName + "@" + ofToString(moduleId);
	
	auto newModule = moduleList[moduleName]();
    newModule->setupModule(moduleName, resolution);
    newModule->gui->setPosition((ofGetMouseX() - translation.x)/zoom - 100/zoom, (ofGetMouseY() - translation.y)/zoom - 15/zoom);
    newModule->setModuleMidiMapMode(midiMapMode);
	newModule->setModuleId(moduleId);
	
    if(moduleName == "SCREEN OUTPUT")
    {
		DKScreenOutput* so = static_cast<DKScreenOutput*>(newModule);;
        so->mainWindow = mainWindow;
        modules.insert({uniqueModuleName, so});
    }
	else if (moduleName == "PROJECT")
	{
		DKConfig* config = static_cast<DKConfig*>(newModule);;
		config->setupModule("PROJECT", resolution);
		config->gui->setPosition(ofGetScreenWidth()/2 - 160, 20);
		config->setModuleMidiMapMode(midiMapMode);
		ofAddListener(config->onResolutionChangeEvent, this, &ofxDarkKnight::onResolutionChange);
		config->setModuleId(getNextModuleId());
		modules.insert({uniqueModuleName, config });
	}
    else if(moduleName == "MIDI CONTROL IN")
    {
        DKMidiControlIn * controller = static_cast<DKMidiControlIn*>(newModule);;
        ofAddListener(controller->sendMidi, this, &ofxDarkKnight::newMidiMessage);
        modules.insert({uniqueModuleName, controller});
    } 
    else
    {
            modules.insert({uniqueModuleName, newModule});
    }

    if(newModule->getModuleHasChild())
    {
        DKMediaPool * mp = static_cast<DKMediaPool*>(newModule);
        mp->setModulesReference(&modules);
        mp->setTranslationReferences(&translation, &zoom);
        int mIndex = 0;
        for(CollectionItem item : mp->collection)
        {
            DKModule * m = item.canvas;
            if(mIndex > 0) m->disable();
            string childName = m->getName();
            m->setModuleMidiMapMode(midiMapMode);
			int childModuleId = getNextModuleId();
			m->setModuleId(childModuleId);
			string childNameWithId = childName + "@" + ofToString(childModuleId);
            modules.insert({childNameWithId, m}); 
            mIndex ++;
        }

    }
    return newModule;
}

void ofxDarkKnight::deleteModule(string moduleName)
{
    modules.erase(moduleName);
}

//delete wires connected to focused component and then delete the component
void ofxDarkKnight::deleteFocusedModule()
{
    //iterate all the modules to get the focused one
    for(pair<string, DKModule*> module : modules )
    {
        //focused module
        if(module.second->gui->getFocused())
        {
            //get all the items of the focused component
            vector<ofxDatGuiComponent*> components = module.second->gui->getItems();
            
            //iterate all the components to compare with the wires list
            for(auto component : components)
            {
                //true if component has children (it's a folder)
                if(component->children.size() > 0)
                {
                    //iterate all component's children
                    for(auto childComponent : component->children)
                    {
                        deleteComponentWires(childComponent, module.second->getModuleId());
                    }
                }
                //component doesn't have children
                else {
                    deleteComponentWires(component, module.second->getModuleId());
                }
            }
            vector<DKWire>::iterator itw = wires.begin();
            while(itw != wires.end())
            {
                if(module.second == itw->outputModule || module.second == itw->inputModule)
                {
                    wires.erase(itw);
                    itw = wires.begin();
                } else {
                    itw++;
                }
            }
            // now that we deleted all the module's wires procede to unmount and delete the module it self
            module.second->inputs.clear();
            module.second->outputs.clear();
            module.second->gui->deleteItems();
            modules.erase(module.first);
            module.second->unMount();
            break;
        }
    }
}

void ofxDarkKnight::deleteAllModules()
{
	for (auto module : modules)
	{
		module.second->inputs.clear();
		module.second->outputs.clear();
		module.second->gui->deleteItems();
		module.second->unMount();
	}

	modules.clear();
}

void ofxDarkKnight::deleteComponentWires(ofxDatGuiComponent * component, int deletedModuleId)
{
    //compare each component with all the stored wires
    vector<DKWire>::iterator itw = wires.begin();
    while(itw != wires.end())
    {
        if((component->getName() == itw->getInput()->getName() ||
           component->getName() == itw->getOutput()->getName()) &&
			(itw->outputModule->getModuleId() == deletedModuleId || 
			 itw->inputModule->getModuleId() == deletedModuleId))
        {
            wires.erase(itw);
            itw = wires.begin();
        } else {
            itw++;
        }
    }
}

void ofxDarkKnight::onResolutionChange(ofVec2f & newResolution)
{
    resolution = newResolution;
    for(pair<string, DKModule*> module : modules )
    {
        module.second->setResolution(newResolution.x, newResolution.y);
        module.second->setup();
    }
}

void ofxDarkKnight::close()
{
    for(pair<string, DKModule*> module : modules )
    {
        module.second->unMount();
    }
    modules.clear();
}

void ofxDarkKnight::newMidiMessage(ofxMidiMessage & msg)
{
    //send midi message to media pool.
    for(pair<string, DKModule*> module : modules )
        if(module.second->getModuleHasChild())
        {
            DKMediaPool * mp = static_cast<DKMediaPool*>(module.second);
            mp->gotMidiMessage(&msg);
        }
    
    string mapping;
    if(msg.control > 0)
    {
        mapping = ofToString(msg.channel) + "/"
                + ofToString(msg.control);
    } else {
        mapping = ofToString(msg.channel) + "/"
                + ofToString(msg.pitch);

        if(msg.status == MIDI_NOTE_ON)
            for(pair<string, DKModule*> module : modules )
                if(module.second->getModuleHasChild())
                {
                    DKMediaPool * mp = static_cast<DKMediaPool*>(module.second);
                    mp->gotMidiMapping(mapping);
                }
    }
}

void ofxDarkKnight::sendMidiMessage(ofxMidiMessage & msg)
{
    darkKnightMidiOut.sendControlChange(msg.channel, msg.control, msg.value);
}


void ofxDarkKnight::savePreset()
{
    for(pair<string, DKModule*> module : modules)
    {
        if(module.second->getModuleHasChild())
        {
            DKMediaPool * mediaPool = static_cast<DKMediaPool*>(module.second);
            mediaPool->savePreset();
        }
    }
}

int ofxDarkKnight::getNextModuleId()
{
	return moduleId++;
}
//   ofxDarkKnight

void ofxDarkKnight::loadProjectFromXml(ofXml xml)
{

}

void ofxDarkKnight::setTranslation(ofVec2f t)
{
	translation = t;
}

void ofxDarkKnight::setZoom(float z)
{
	zoom = z;
}

unordered_map<string, DKModule*>* ofxDarkKnight::getModulesReference()
{
	return &modules;
}

vector<DKWire>* ofxDarkKnight::getWiresReference()
{
	return &wires;
}

ofVec2f* ofxDarkKnight::getTranslationReference()
{
	return &translation;
}

float* ofxDarkKnight::getZoomReference()
{
	return &zoom;
}

void ofxDarkKnight::resizeWindow(int w, int h)
{
    float browserWidth = w * 0.8;
    float browserHeight = h * 0.8;
    
    delete componentsList;
    componentsList = nullptr;
    
    componentsList = new ofxDatGuiScrollView("MODULES", 11);
    componentsList->setWidth(browserWidth / 3);
    componentsList->setHeight(poolNames.size() * 25.0);
    componentsList->setPosition(ofGetWidth()/2 - 0.5 * browserWidth / 3, ofGetHeight()/2 - 200);
    componentsList->onScrollViewEvent(this, &ofxDarkKnight::onComponentListChange);
    
    
    componentsList->setWidth(browserWidth / 3);
    componentsList->setHeight(poolNames.size() * 25.0);
    componentsList->setPosition(ofGetWidth()/2 - 0.5 * browserWidth / 3, ofGetHeight()/2 - 200);
    for(auto it = poolNames.begin(); it != poolNames.end(); it++) componentsList->add(*it);
}
