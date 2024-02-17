#include "pippi.h"

int main() {
    // Dramatis personae
    lpnode_t * osc;
    lpnode_t * mod1;
    lpnode_t * mod2;
    lpnode_t * mod3;
    lpbuffer_t * out;
    lpfloat_t sample;
    lpfloat_t sr = 48000.f;
    size_t i, length;

    // Set the output length in seconds
    length = (size_t)(sr * 10.f);

    // Create the output buffer
    out = LPBuffer.create(length, 2, (int)sr);

    // Create the carrier node
    osc = LPNode.create(NODE_SINEOSC);
    osc->params.sineosc->samplerate = sr;

    // Create the first mod node
    mod1 = LPNode.create(NODE_SINEOSC);
    mod1->params.sineosc->samplerate = sr;

    // Create the second mod node
    mod2 = LPNode.create(NODE_SINEOSC);
    mod2->params.sineosc->samplerate = sr;

    // Create the third mod node
    mod3 = LPNode.create(NODE_SINEOSC);
    mod3->params.sineosc->samplerate = sr;

    // Connect the output of the modular node to 
    // the freq param input of the carrier node.
    // Configure the modulator output to scale from 
    // 100hz to 300hz.
    LPNode.connect(osc, NODE_PARAM_FREQ, mod1, 60, 300);

    // Connect the output of the second modulator node 
    // to the freq param of the first modulator node, 
    // ...and the second to the third...
    LPNode.connect(mod1, NODE_PARAM_FREQ, mod2, 10, 400);
    LPNode.connect(mod2, NODE_PARAM_FREQ, mod3, 1500, 4050);

    // Set the freq param on the modulator to a fixed value of
    // 200hz. Internally this creates a fixed signal node and 
    // connects it to the given param with a fixed node->last 
    // value. (NODE_SIGNAL)
    LPNode.connect_signal(mod3, NODE_PARAM_FREQ, 0.15f);

    // Render some samples to the output buffer
    for(i=0; i < length; i++) {
        // The processing order here could be important 
        // for some feedback scenarios, but in this example
        // it doesn't really make a difference.

        // Process the carrier node, which 
        // uses the mod->last value for the 
        // frequency input.
        sample = LPNode.process(osc);

        // Process the modulator nodes, which 
        // updates the mod->last values with the 
        // current output value.
        LPNode.process(mod1);
        LPNode.process(mod2);
        LPNode.process(mod3);

        out->data[i * 2] = sample;
        out->data[i * 2 + 1] = sample;
    }

    // Write the output buffer to disk
    LPSoundFile.write("renders/nodeosc-out.wav", out);

    // Clean up the mess
    LPNode.destroy(mod1);
    LPNode.destroy(mod2);
    LPNode.destroy(mod3);
    LPNode.destroy(osc);
    LPBuffer.destroy(out);
}


