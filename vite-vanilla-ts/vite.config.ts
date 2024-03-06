import klausPlugin from './src/vite-plugin-klaus'
import Inspect from 'vite-plugin-inspect'

export default {
  plugins: [Inspect(), klausPlugin()],
}