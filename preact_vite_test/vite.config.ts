import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'
import {viteSingleFile} from "./builder/vite-plugin-singlefile";

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [preact(), viteSingleFile()],
  build:{
    minify:true,
    cssCodeSplit:false,
  }
})
