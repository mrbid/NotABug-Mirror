#ifndef SADPLY_H
#define SADPLY_H

const GLfloat sad_vertices[] = {-0.154828,0.781221,0.000000,0.345174,0.715387,0.000000,0.103988,0.792711,0.000000,-0.345162,0.715387,0.000000,0.509466,0.604598,0.000000,-0.509453,0.604598,0.000000,0.000006,0.646067,0.000000,-0.124660,0.634898,0.000000,0.124672,0.634898,0.000000,-0.278081,0.581728,0.000000,0.278094,0.581729,0.000000,0.640004,0.456254,0.000000,-0.639992,0.456254,0.000000,0.410625,0.492283,0.000000,-0.410613,0.492283,0.000000,0.515991,0.372578,0.000000,-0.515978,0.372578,0.000000,0.747129,0.229660,0.000000,-0.729224,0.278017,0.000000,0.602506,0.189838,0.000000,-0.602493,0.189838,0.000000,-0.281081,0.269232,0.000000,-0.159433,0.225635,0.000000,-0.221863,0.271828,0.000000,0.215641,0.269232,0.000000,0.337288,0.225635,0.000000,0.274859,0.271828,0.000000,-0.323779,0.240535,0.000000,0.172942,0.240535,0.000000,-0.769895,0.077782,0.000000,-0.346804,0.194277,0.000000,0.149917,0.194277,0.000000,-0.149904,0.154285,0.000000,0.346817,0.154285,0.000000,-0.337276,0.122927,0.000000,0.159445,0.122927,0.000000,-0.172930,0.108027,0.000000,0.305909,0.088038,0.000000,0.769908,-0.027191,0.000000,0.620895,-0.016939,0.000000,-0.620883,-0.016940,0.000000,-0.215628,0.079330,0.000000,-0.274846,0.076734,0.000000,0.221875,0.076734,0.000000,-0.747116,-0.179068,0.000000,-0.588035,-0.178257,0.000000,0.588048,-0.178256,0.000000,0.729237,-0.227425,0.000000,-0.122224,-0.137169,0.000000,0.164551,-0.155687,0.000000,0.046399,-0.124823,0.000000,-0.268598,-0.219071,0.000000,0.268610,-0.219071,0.000000,-0.515978,-0.321986,0.000000,0.515991,-0.321986,0.000000,-0.340676,-0.304387,0.000000,0.009955,-0.268723,0.000000,0.342345,-0.309100,0.000000,-0.639992,-0.405662,0.000000,0.640004,-0.405662,0.000000,-0.142548,-0.308705,0.000000,0.150568,-0.314689,0.000000,-0.329387,-0.361213,0.000000,0.310525,-0.385542,0.000000,0.410625,-0.441691,0.000000,-0.410612,-0.441691,0.000000,0.234227,-0.393180,0.000000,-0.234761,-0.393598,0.000000,-0.296628,-0.390968,0.000000,-0.509453,-0.554006,0.000000,0.509466,-0.554006,0.000000,0.278094,-0.531136,0.000000,-0.278081,-0.531136,0.000000,0.083677,-0.593589,0.000000,-0.124660,-0.584306,0.000000,-0.345162,-0.664794,0.000000,0.345174,-0.664795,0.000000,-0.154828,-0.730628,0.000000,0.103987,-0.742119,0.000000};
const GLushort sad_indices[] = {0,1,2,3,1,0,3,4,1,5,6,3,6,4,3,5,7,6,8,4,6,5,9,7,10,4,8,10,11,4,12,9,5,13,11,10,12,14,9,15,11,13,12,16,14,15,17,11,18,16,12,19,17,15,18,20,16,21,22,23,24,25,26,27,22,21,28,25,24,29,20,18,30,22,27,31,25,28,30,32,22,31,33,25,34,32,30,35,33,31,34,36,32,35,37,33,19,38,17,39,38,19,29,40,20,34,41,36,42,41,34,43,37,35,44,40,29,44,45,40,46,38,39,46,47,38,48,49,50,51,49,48,51,52,49,44,53,45,54,47,46,55,52,51,55,56,52,56,57,52,58,53,44,54,59,47,55,60,56,61,57,56,62,60,55,61,63,57,64,59,54,58,65,53,66,63,61,62,67,60,68,67,62,69,65,58,64,70,59,71,70,64,69,72,65,73,70,71,69,74,72,75,74,69,73,76,70,75,73,74,75,76,73,77,76,75,77,78,76};
const GLsizei sad_numind = 219;
const GLsizei sad_numvert = 79;

#endif