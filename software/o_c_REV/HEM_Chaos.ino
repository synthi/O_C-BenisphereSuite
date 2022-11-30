/ Copyright (c) 2018, Tom Whiston
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


// From maxpatch
// should be able to set params with input. which make the most sense?
// todo - other algos and choose on screen

#define CHAOS_AVAILABLE_ALGO 1

class ChaosAlgo {

    public:

        void cvIn(int a, int b){};

        // Called when clocked in input 0
        virtual void run();

        simfloat getX(){
            return this->x;
        }
        simfloat getY(){
            return this->y;
        }

    protected:
        simfloat x;
        simfloat y;
        simfloat z;
};

// TODO - for some reason nothing is called right now
class ChaoSimplified : public ChaosAlgo {

public:

    ChaoSimplified(){
        // todo, how?
        // this->param_r = 30.5;
        // this->param_TS = 0.0003;
        this->param_r = int2simfloat(305)/10;
        this->param_TS = int2simfloat(3)/10000;

        this->param_p = int2simfloat(10);
        this->param_b = int2simfloat(8)/3;

        //x 0.1, y -10., z 0.1
        this->x = int2simfloat(1)/10;
        this->y = int2simfloat(-10);
        this->z = int2simfloat(1)/10;
    }

    void run(){
	        //calculate change
	        simfloat dx = this->param_p * (this->y - this->x);
	        simfloat dy = (this->x * (this->param_r - this->z)) - this->y;
	        simfloat dz = (this->x * this->y) - (this->param_b * this->z);
	        //update histories
	        this->x = this->x + dx * this->param_TS;
	        this->y = this->y + dy * this->param_TS;
	        this->z = this->z + dz * this->param_TS;
    };

    private:
        // in max params can be set from outside the dsp code
        simfloat param_p;
        simfloat param_b;
        simfloat param_r;
        simfloat param_TS;
};


class Chaos : public HemisphereApplet {
public:

    Chaos() : algos{new ChaoSimplified()}{}

    const char* applet_name() { // Maximum 10 characters
        return "Chaos";
    }

	/* Run when the Applet is selected */
    void Start() {
        this->selectedAlgo = 0;
    }


    long Map(simfloat x, long in_min, long in_max, long out_min, long out_max) {
        //return (simfloat2int(x) - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        return simfloat2int(x) / 20;
    }

	/* Run during the interrupt service routine, 16667 times per second */
    void Controller() {
        if (Clock(0)) {	
	        this->algos[selectedAlgo]->run();
            int xo = Map(this->algos[selectedAlgo]->getX(), -(__INT32_MAX__ >> 14), __INT32_MAX__ >> 14, -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV) ;
            int yo = Map(this->algos[selectedAlgo]->getY(), -(__INT32_MAX__ >> 14), __INT32_MAX__ >> 14, -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV) ;
            Out(0, xo);
            Out(1, yo);
        }
    }

	/* Draw the screen */
    void View() {
        int xo = Map(this->algos[selectedAlgo]->getX(), -(__INT32_MAX__ >> 14), __INT32_MAX__ >> 14, -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV) ;
        int yo = Map(this->algos[selectedAlgo]->getY(), -(__INT32_MAX__ >> 14), __INT32_MAX__ >> 14, -HEMISPHERE_3V_CV, HEMISPHERE_MAX_CV) ;
            
        gfxHeader(applet_name());
        gfxSkyline();
        // Add other view code as private methods
        gfxPrint(0, 15, "x ");
        gfxPrint(xo);
        gfxPrint(0, cursorYpos(drawBase, 1), "y ");
        gfxPrint(yo);

    }

	/* Called when the encoder button for this hemisphere is pressed */
    void OnButtonPress() {
    }

	/* Called when the encoder for this hemisphere is rotated
	 * direction 1 is clockwise
	 * direction -1 is counterclockwise
	 */
    void OnEncoderMove(int direction) {
    }
        
    /* Each applet may save up to 32 bits of data. When data is requested from
     * the manager, OnDataRequest() packs it up (see HemisphereApplet::Pack()) and
     * returns it.
     */
    uint32_t OnDataRequest() {
        uint32_t data = 0;
        // example: pack property_name at bit 0, with size of 8 bits
        // Pack(data, PackLocation {0,8}, property_name); 
        return data;
    }

    /* When the applet is restored (from power-down state, etc.), the manager may
     * send data to the applet via OnDataReceive(). The applet should take the data
     * and unpack it (see HemisphereApplet::Unpack()) into zero or more of the applet's
     * properties.
     */
    void OnDataReceive(uint32_t data) {
        // example: unpack value at bit 0 with size of 8 bits to property_name
        // property_name = Unpack(data, PackLocation {0,8}); 
    }

protected:

    const uint8_t drawBase = 15;
    const uint8_t lineHeight = 10;

    uint8_t cursorYpos(uint8_t pos, uint8_t line = 0, int8_t offset = 0){
        return (drawBase + (line * lineHeight)) + offset;
    }
    /* Set help text. Each help section can have up to 18 characters. Be concise! */
    void SetHelp() {
        //                               "------------------" <-- Size Guide
        help[HEMISPHERE_HELP_DIGITALS] = "Digital in help";
        help[HEMISPHERE_HELP_CVS]      = "CV in help";
        help[HEMISPHERE_HELP_OUTS]     = "Out help";
        help[HEMISPHERE_HELP_ENCODER]  = "123456789012345678";
        //                               "------------------" <-- Size Guide
    }
    
private:

    // TODO should be an array of the base class so we can select one and run it in a clean way
    uint8_t selectedAlgo;
    ChaosAlgo* algos[CHAOS_AVAILABLE_ALGO];

};


////////////////////////////////////////////////////////////////////////////////
//// Hemisphere Applet Functions
///
///  Once you run the find-and-replace to make these refer to Chaos,
///  it's usually not necessary to do anything with these functions. You
///  should prefer to handle things in the HemisphereApplet child class
///  above.
////////////////////////////////////////////////////////////////////////////////
Chaos Chaos_instance[2];

void Chaos_Start(bool hemisphere) {Chaos_instance[hemisphere].BaseStart(hemisphere);}
void Chaos_Controller(bool hemisphere, bool forwarding) {Chaos_instance[hemisphere].BaseController(forwarding);}
void Chaos_View(bool hemisphere) {Chaos_instance[hemisphere].BaseView();}
void Chaos_OnButtonPress(bool hemisphere) {Chaos_instance[hemisphere].OnButtonPress();}
void Chaos_OnEncoderMove(bool hemisphere, int direction) {Chaos_instance[hemisphere].OnEncoderMove(direction);}
void Chaos_ToggleHelpScreen(bool hemisphere) {Chaos_instance[hemisphere].HelpScreen();}
uint32_t Chaos_OnDataRequest(bool hemisphere) {return Chaos_instance[hemisphere].OnDataRequest();}
void Chaos_OnDataReceive(bool hemisphere, uint32_t data) {Chaos_instance[hemisphere].OnDataReceive(data);}
