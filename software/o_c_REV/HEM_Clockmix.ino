// Hemisphere Applet Boilerplate. Follow these steps to add a Hemisphere app:
//
// (1) Save this file as HEM_ClockMix.ino
// (2) Find and replace "ClassName" with the name of your Applet class
// (3) Implement all of the public methods below
// (4) Add text to the help section below in SetHelp()
// (5) Add a declare line in hemisphere_config.h, which looks like this:
//     DECLARE_APPLET(id, categories, ClockMix), \
// (6) Increment HEMISPHERE_AVAILABLE_APPLETS in hemisphere_config.h
// (7) Add your name and any additional copyright info to the block below

// Copyright (c) 2018, __________
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <Entropy.h>

#define HEM_CLOCKMIX_MAX 100
#define HEM_CLOCKMIX_GUI_ELEMENTS 2

// TODO 
//      - cv input to control probability
//      - GUI
//      - save and restore values
class ClockMix : public HemisphereApplet {
public:

    const char* applet_name() {
        return "ClockMix";
    }

    void Start() {
        Entropy.Initialize();
        in1 = false;
        in2 = false;
    }

    void Controller() {

        in1 = processClock(0);
        in2 = processClock(1);

        //if either outs are true then output a tick
        if (in1 || in2)
        {
            ClockOut(0);
        }
        // if both are high output
        if (in1 && in2)
        {
            ClockOut(1);
        }

    }

    void View() {
        gfxHeader(applet_name());
        DrawInterface();
    }

    void OnButtonPress() {
        if (++cursor == HEM_CLOCKMIX_GUI_ELEMENTS) cursor = 0;
        ResetCursor();
    }

    void OnEncoderMove(int direction) {
        prob[cursor] += direction;
        if (prob[cursor] > HEM_CLOCKMIX_MAX) prob[cursor] = HEM_CLOCKMIX_MAX;
        if (prob[cursor] <= 0) prob[cursor] = 0;
    }
        
    uint32_t OnDataRequest() {
        uint32_t data = 0;
        // example: pack property_name at bit 0, with size of 8 bits
        // Pack(data, PackLocation {0,8}, property_name); 
        return data;
    }

    void OnDataReceive(uint32_t data) {
        // example: unpack value at bit 0 with size of 8 bits to property_name
        // property_name = Unpack(data, PackLocation {0,8}); 
    }

protected:
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "Clocks in";
        help[HEMISPHERE_HELP_CVS]      = "NOTHING";
        help[HEMISPHERE_HELP_OUTS]     = "Clocks || / &&";
        help[HEMISPHERE_HELP_ENCODER]  = "Set values";
        //                               "------------------" <-- Size Guide
    }
    
private:
    int cursor; // Which output is currently being edited

    uint32_t prob[2]; //probability for each input to be sent to the output

    bool in1;
    bool in2;

    // faster than a modulo division
    // uint32_t reduce(uint32_t x, uint32_t N) {
    //     return ((uint64_t) x * (uint64_t) N) >> 32 ;
    // }

    inline bool processClock(int ch){
        bool state = false;
        if (Clock(ch)) {
            uint32_t r = Entropy.random(0, HEM_CLOCKMIX_MAX);
            if (r <= prob[ch])
            {
                state = true;
            }
        }
        return state;
    }

    const uint8_t drawBase = 15;
    const uint8_t lineHeight = 10;
    
    void DrawInterface() {
        gfxSkyline();

        gfxPrint(0, drawBase, "prob A");
        gfxPrint(this->prob[0]);
        if (cursor == 0) {
            uint8_t ypos = cursorYpos(drawBase,1,-2);
            gfxLine(0, ypos, 50, ypos);
        }
        gfxPrint(0, cursorYpos(drawBase, 1), "Prob B");
        gfxPrint(this->prob[1]);
        if (cursor == 1) {
            uint8_t ypos = cursorYpos(drawBase,2,-2);
            gfxLine(0, ypos, 50, ypos);
        }
    }

    uint8_t cursorYpos(uint8_t pos, uint8_t line = 0, int8_t offset = 0){
        return (drawBase + (line * lineHeight)) + offset;
    }

};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to ClockMix,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
ClockMix ClockMix_instance[2];

void ClockMix_Start(bool hemisphere) {ClockMix_instance[hemisphere].BaseStart(hemisphere);}
void ClockMix_Controller(bool hemisphere, bool forwarding) {ClockMix_instance[hemisphere].BaseController(forwarding);}
void ClockMix_View(bool hemisphere) {ClockMix_instance[hemisphere].BaseView();}
void ClockMix_OnButtonPress(bool hemisphere) {ClockMix_instance[hemisphere].OnButtonPress();}
void ClockMix_OnEncoderMove(bool hemisphere, int direction) {ClockMix_instance[hemisphere].OnEncoderMove(direction);}
void ClockMix_ToggleHelpScreen(bool hemisphere) {ClockMix_instance[hemisphere].HelpScreen();}
uint32_t ClockMix_OnDataRequest(bool hemisphere) {return ClockMix_instance[hemisphere].OnDataRequest();}
void ClockMix_OnDataReceive(bool hemisphere, uint32_t data) {ClockMix_instance[hemisphere].OnDataReceive(data);}
