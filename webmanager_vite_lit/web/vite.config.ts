import { defineConfig } from 'vite'
import {viteSingleFile} from "./vite_plugin/vite-plugin-singlefile"


// https://vitejs.dev/config/
export default defineConfig({
  plugins: [viteSingleFile()],
  build:{
    minify:true,
    cssCodeSplit:false,
  },
  server:{
    proxy:{
      "/webmanager_ws":{
        target:"ws://localhost:3000",
        ws:true,
      }
    }
  }
})