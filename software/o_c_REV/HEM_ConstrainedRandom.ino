// Copyright (c) 2018, Tom Whiston
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

// TODO
// add option to use digital input 2 to change the value, and 1 just to clock
// add option to quantise output values
// do saving with better values and capture all settings
// fix overflow issue when min and max are inverted. close it down to a single window which you can 'push' instead
//
// The real range is -HEMISPHERE_3V_CV, 0, HEMISPHERE_MAX_CV

#include <Entropy.h>
static const uint16_t const_rand_step_vals[] = {1,2,5,10,25,50,100,250,500,1000,1500,2000};
static const uint8_t const_rand_step_count = sizeof(const_rand_step_vals)/sizeof(const_rand_step_vals[0])-1;
static bool randSetup = false;

class ConstrainedRandom : public HemisphereApplet {
public:

    const char* applet_name() { // Maximum 10 characters
        return "ConstRand";
    }

	/* Run when the Applet is selected */
    void Start() {
      if(randSetup == false){
        Entropy.Initialize();
        randSetup = true;
      }
      //TODO - restore things instead of resetting
      this->randMax = HEMISPHERE_MAX_CV;
      this->newRand();
    }

	/* Run during the interrupt service routine, 16667 times per second */
    void Controller() {

        if (Changed(0)) {
          this->randMin = constrain(In(0), -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV);
        }
        if (Changed(1)) {
          this->randMax = constrain(In(1), -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV);
        }

        // dont wait for lag as we dont read any adc values
        if (Clock(0)) {
            //output the previous value on the second output
            Out(1, this->randValue);
            if(!this->changeInputActive){
                this->newRand();
            }
            Out(0, this->randValue);
        }
        if(Clock(1) && this->changeInputActive){
            this->newRand();
        }
    }

	/* Draw the screen */
    void View() {
        gfxHeader(applet_name());
        DrawUI();
    }

	/* Called when the encoder button for this hemisphere is pressed */
    void OnButtonPress() {
        //cursor = !cursor;
        if (++cursor == 3) cursor = 0;
    }

	/* Called when the encoder for this hemisphere is rotated
	 * direction 1 is clockwise
	 * direction -1 is counterclockwise
	 */
    void OnEncoderMove(int direction) {
        switch (cursor)
        {
        case 0:
            this->randMin = constrain(this->randMin + (direction * const_rand_step_vals[this->valueIndex]), -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV);
            break;
        case 1:
            this->randMax = constrain(this->randMax + (direction * const_rand_step_vals[this->valueIndex]), -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV);
            break;
        case 2:
            this->valueIndex = constrain(this->valueIndex + direction, 0, const_rand_step_count);
            break;
        default:
            break;
        }

    }
        
    /* Each applet may save up to 32 bits of data. When data is requested from
     * the manager, OnDataRequest() packs it up (see HemisphereApplet::Pack()) and
     * returns it.
     */
    uint32_t OnDataRequest() {
        uint32_t data = 0;
        Pack(data, PackLocation {0,16}, this->randMin);
        Pack(data, PackLocation {16,16}, this->randMax);
        return data;
    }

    /* When the applet is restored (from power-down state, etc.), the manager may
     * send data to the applet via OnDataReceive(). The applet should take the data
     * and unpack it (see HemisphereApplet::Unpack()) into zero or more of the applet's
     * properties.
     */
    void OnDataReceive(uint32_t data) {
        this->randMin = Unpack(data, PackLocation {0,16});
        this->randMax = Unpack(data, PackLocation {16,16});
    }

protected:
    /* Set help text. Each help section can have up to 18 characters. Be concise! */
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "clock / new value";
        help[HEMISPHERE_HELP_CVS]      = "min / max";
        help[HEMISPHERE_HELP_OUTS]     = "Rand / last rand";
        help[HEMISPHERE_HELP_ENCODER]  = "set min/max values";
        //                               "------------------" <-- Size Guide
    }
    
private:

    int cursor = 0;
    int randMin = 0;
    int randMax = 0;
    int randValue = 0;
    int valueIndex = 0;

    bool changeInputActive = false;

    const uint8_t drawBase = 15;
    const uint8_t lineHeight = 10;

    void newRand(){
        this->randValue = Entropy.random(this->randMin,this->randMax);
    }

    void DrawUI(){

        gfxPrint(0, drawBase, "min ");
        gfxPrint(this->randMin);
        if (cursor == 0) {
            uint8_t ypos = cursorYpos(drawBase,1,-2);
            gfxLine(0, ypos, 50, ypos);
        }
        gfxPrint(0, cursorYpos(drawBase, 1), "max ");
        gfxPrint(this->randMax);
        if (cursor == 1) {
            uint8_t ypos = cursorYpos(drawBase,2,-2);
            gfxLine(0, ypos, 50, ypos);
        }
        gfxPrint(0, cursorYpos(drawBase, 2), "inc ");
        gfxPrint(const_rand_step_vals[this->valueIndex]);
        if (cursor == 2) {
            uint8_t ypos = cursorYpos(drawBase,3,-2);
            gfxLine(0, ypos, 50, ypos);
        }
        gfxPrint(0, cursorYpos(drawBase, 3), "val ");
        gfxPrint(this->randValue);

    }

    uint8_t cursorYpos(uint8_t pos, uint8_t line = 0, int8_t offset = 0){
        return (drawBase + (line * lineHeight)) + offset;
    }

};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
////////////////////////////////////////////////////////////////////////////////
ConstrainedRandom ConstrainedRandom_instance[2];

void ConstrainedRandom_Start(bool hemisphere) {ConstrainedRandom_instance[hemisphere].BaseStart(hemisphere);}
void ConstrainedRandom_Controller(bool hemisphere, bool forwarding) {ConstrainedRandom_instance[hemisphere].BaseController(forwarding);}
void ConstrainedRandom_View(bool hemisphere) {ConstrainedRandom_instance[hemisphere].BaseView();}
void ConstrainedRandom_OnButtonPress(bool hemisphere) {ConstrainedRandom_instance[hemisphere].OnButtonPress();}
void ConstrainedRandom_OnEncoderMove(bool hemisphere, int direction) {ConstrainedRandom_instance[hemisphere].OnEncoderMove(direction);}
void ConstrainedRandom_ToggleHelpScreen(bool hemisphere) {ConstrainedRandom_instance[hemisphere].HelpScreen();}
uint32_t ConstrainedRandom_OnDataRequest(bool hemisphere) {return ConstrainedRandom_instance[hemisphere].OnDataRequest();}
void ConstrainedRandom_OnDataReceive(bool hemisphere, uint32_t data) {ConstrainedRandom_instance[hemisphere].OnDataReceive(data);}
