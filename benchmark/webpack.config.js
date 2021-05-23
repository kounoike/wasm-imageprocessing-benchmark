const path = require('path');
module.exports = {
  entry: "./index.js",
  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "index.js",
  },
  devServer: {
    port: 8000,
    allowedHosts: [
      '.ngrok.io'
    ]
  },
  mode: "development"
};