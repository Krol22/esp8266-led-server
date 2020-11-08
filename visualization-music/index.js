// const portAudio = require("naudiodon");
//
// const client = dgram.createSocket("udp4");
//
// const audioIO = new portAudio.AudioIO({
  // inOptions: {
    // channelCount: 1,
    // sampleFormat: portAudio.SampleFormat16Bit,
    // sampleRate: 44100,
    // deviceId: -1, // Use -1 or omit the deviceId to select the default device
    // closeOnError: true // Close the stream if an audio error is detected, if set false then just log the error
  // },
// });
//
// audioIO.on('data', buf => {
  // c.clear();
  // const data = Uint8Array.from(buf);
  // const averageData = new Uint8Array(1);
  // for (let i = 0; i < buf.length - 50; i+=50) {
    // let value = 0;
    // for (let j = 0; j < 50; j++) {
      // adder = data[i + j] > 30 ? data[i + j] : 0;
      // value += data[i + j]
    // }
//
    // averageData[i / 50] = value / 50;
  // }
  // // let sum = 0;
  // // for (let i = 0; i < buf.length; i++) {
    // // sum += data[i];
  // // }
// //
  // // averageData[0] = sum / buf.length;
  // // client.send(averageData, 7777, '192.168.0.150');
// });
//
// audioIO.start();
//
//

const dgram = require("dgram");
const coreAudio = require("node-core-audio");
const fourierTransform = require("fourier-transform");
const BUFFER_SIZE = 256;
const client = dgram.createSocket("udp4");

const engine = coreAudio.createNewAudioEngine();

function processAudio( inputBuffer ) {
  const averageData = new Uint8Array(100);
  const buffer = inputBuffer;
  const test = Float32Array.from(inputBuffer[0]);
  const spectrum = fourierTransform(test);

  for (let i = 0; i < spectrum.length - 10; i+=10) {
    let value = 0;
    for (let j = 0; j < 10; j++) {
      value += (spectrum[i + j] * 500000);
    }

  // for (let i = 0; i < buffer[0].length - 10; i+=10) {
    // let value = 0;
    // for (let j = 0; j < 10; j++) {
      // value += (buffer[0][i + j] * 100);
    // }

    averageData[i / 10] = value / 10;
  }
  client.send(averageData, 7777, '192.168.0.150');
}

const gain = 100;
let ft;

function calcMean(arr) {
  let sum = 0;
  for (let i = 0; i < arr.length; i++) {
      sum += arr[i];
  }
  return sum / arr.length;
}

function normalize(ft) {
  const mean = calcMean(ft);
  for (let i = 0; i < ft.length; i++) {
    let normalization = 0;
    if (this.normalizationEnabled) {
      let multiples = ft[i] / mean;
      if (7 < multiples) {
          normalization = ft[i] / 1.4;
      } else if (6 < multiples) {
          normalization = ft[i] / 1.8;
      } else if (5 < multiples) {
          normalization = ft[i] / 2;
      } else if (4 < multiples) {
          normalization = ft[i] / 3;
      } else if (3 < multiples) {
          normalization = ft[i] / 4;
      } else if (2 < multiples) {
          normalization = ft[i] / 5;
      } else if (1 < multiples) { 
          normalization = ft[i] / 7;
      }
    }
    ft[i] = (ft[i] - normalization) * gain;
  }
}

function normalizeTs(ts) {
  for (let i = 0; i < ts.length; i++) {
    ts[i] = ts[i] / 100 * gain;
  }
  return ts;
}

function sendSample(inputBuffer) {
  const ts = normalizeTs(inputBuffer[0]);
  ft = fourierTransform(ts);

  normalize(ft);
}

function processAudio2(inputBuffer) {
  sendSample(inputBuffer);
  console.log(ft);
}

engine.addAudioCallback(processAudio);

// setInterval(() => {
  // const averageData = new Uint8Array(100);
  // const buffer = engine.read();
  // for (let i = 0; i < buffer[0].length - 10; i+=10) {
    // let value = 0;
    // for (let j = 0; j < 10; j++) {
      // value += buffer[0][i + j] * 100;
    // }
//
    // averageData[i / 10] = value / 10;
  // }
//
  // client.send(averageData, 7777, '192.168.0.150');
// }, 10)
