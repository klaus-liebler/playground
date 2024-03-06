import {InputOptions} from 'rollup';;
export default ()=> {
    return {
      name: 'vite-plugin-klaus', // this name will show up in logs and errors
      version:"0.0.1",
      buildStart(options: InputOptions){
        console.log(`Hallo buildStart Klaus`);
      }
    };
  }