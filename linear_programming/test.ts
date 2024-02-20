import { isVariableDeclaration } from "typescript";
const lpsolver = require("javascript-lp-solver/src/solver");

const obj: any ={};

const hours = 24;
//Daten für Dezember Verbrauch - typischer Tag im Dezeber
const e_v = [19.64,19.04,17.22,18.19,20.83,23.29,31.64,45.06,48.03,45.88,42.74,37.15,36.87,44.64,40.13,38.6,33.06,34.54,34.64,26.25,21.5,18.32,18.5,19.71];

//irgendwelche Daten der erwartete PV-Einspeisung 24.01
const e_pv = [0,0,0,0,0,0,0,0,2.847,12.187,15.235,16.376,15.015,12.356,9.793,6.581,3.355,0.359,0,0,0,0,0,0];

//Strompreis für den 24.01
const preis = [11.19,10.6,10.3,10.32,10.32,10.09,10.11,10,9.73,9.46,8.79,8.8,8.46,6.65,5.67,6.22,8,10.2,11.55,12.58,13,12.87,12.79,12.2];

const e_bn_value = 200;
const e_l_max = 32.6;
const e_e_max =  50;
const q_normal = 6269.78;
const q_nominal = 15665.46;
const q_alt = 21935.25; //Grenze von Z_Temp
const soc_start = 0.4;
const soc_final = 0.7;
const soc_min = 0.2;
const soc_max = 0.9;


const constraints: any = {};
const variables: any = {};

var _obj1 = 
{
    "name": "Strompreis",
    "optimize": "objective",
    "opType": "min",
    "constraints": {
        "soc_0": {
             "equal":soc_start
        },
        "soc_1": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_2": {
            "max": soc_max,
            "min":soc_min
        },
        "soc_3": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_4": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_5": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_6": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_7": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_8": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_9": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_10": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_11": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_12": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_13": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_14": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_15": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_16": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_17": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_18": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_29": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_20": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_21": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_22": {
            "max": soc_max,
            "min": soc_min
        },
        "soc_23": {
            "equal": soc_final
        },
        "objective": {
            "max": q_alt,
        }

    },
    "variables": {
        "soc_0": {
            "objective": parseFloat((2*e_bn_value*preis[0]).toFixed(2)),
            
        },
        "soc_1": {
            "objective": parseFloat((2*e_bn_value*preis[1]).toFixed(2)),
            
        },
        "soc_2": {
            "objective": parseFloat((2*e_bn_value*preis[2]).toFixed(2)),
            
        },
        "soc_3": {
            "objective": parseFloat((2*e_bn_value*preis[3]).toFixed(2)),
            
        },
        "soc_4": {
            "objective": parseFloat((2*e_bn_value*preis[4]).toFixed(2)),
            
        },
        "soc_5": {
            "objective": parseFloat((2*e_bn_value*preis[5]).toFixed(2)),
            
        },
        "soc_6": {
            "objective": parseFloat((2*e_bn_value*preis[6]).toFixed(2)),
            
        },
        "soc_7": {
            "objective": parseFloat((2*e_bn_value*preis[7]).toFixed(2)),
            
        },
        "soc_8": {
            "objective": parseFloat((2*e_bn_value*preis[8]).toFixed(2)),
            
        },
        "soc_9": {
            "objective": parseFloat((2*e_bn_value*preis[9]).toFixed(2)),
            
        },
        "soc_10": {
            "objective": parseFloat((2*e_bn_value*preis[10]).toFixed(2)),
            
        },
        "soc_11": {
            "objective": parseFloat((2*e_bn_value*preis[11]).toFixed(2)),
            
        },
        "soc_12": {
            "objective": parseFloat((2*e_bn_value*preis[12]).toFixed(2)),
            
        },
        "soc_13": {
            "objective": parseFloat((2*e_bn_value*preis[13]).toFixed(2)),
            
        },
        "soc_14": {
            "objective": parseFloat((2*e_bn_value*preis[14]).toFixed(2)),
            
        },
        "soc_15": {
            "objective": parseFloat((2*e_bn_value*preis[15]).toFixed(2)),
            
        },
        "soc_16": {
            "objective": parseFloat((2*e_bn_value*preis[16]).toFixed(2)),
            
        },
        "soc_17": {
            "objective": parseFloat((2*e_bn_value*preis[17]).toFixed(2)),
            
        },
        "soc_18": {
            "objective": parseFloat((2*e_bn_value*preis[18]).toFixed(2)),
            
        },
        "soc_19": {
            "objective": parseFloat((2*e_bn_value*preis[19]).toFixed(2)),
            
        },
        "soc_20": {
            "objective": parseFloat((2*e_bn_value*preis[20]).toFixed(2)),
            
        },
        "soc_21": {
            "objective": parseFloat((2*e_bn_value*preis[21]).toFixed(2)),
            
        },
        "soc_22": {
            "objective": parseFloat((2*e_bn_value*preis[22]).toFixed(2)),
            
        },
        "soc_23": {
            "objective": parseFloat((2*e_bn_value*preis[23]).toFixed(2)),
            
        },
        
    }
};

//Beispiel
var _obj3 = {
        "name": "Quadratic Optimization 1",
        "constraints" : {
            "cst1": { "min": 3 },
            "cst2": { "max": 7 },
            "cst3": { "min": 1 },
            "cst4": { "min": 1 },
            "fritzJohn1": { "equal": 8 },
            "fritzJohn2": { "equal": 8 }
        },
        "variables" : {
            "x": { "cst1" : 1, "cst2" : 1, "cst3" : 1, "fritzJohn1" : 2 },
            "y": { "cst1" : 1, "cst2" : 1, "cst4" : 1, "fritzJohn2" : 2 },
            "lambda1": { "fritzJohn1" : -1, "fritzJohn2" : -1 },
            "lambda2": { "fritzJohn1" :  1, "fritzJohn2" :  1 },
            "lambda3": { "fritzJohn1" : -1 },
            "lambda4": { "fritzJohn2" : -1 }
        }
  
};
var _obj2 = {"name":"Fertilizer","constraints":{"W":{"equal":40,"weight":1,"priority":2},"N":{"equal":50,"weight":1,"priority":1},"P":{"equal":20,"weight":2,"priority":1},"K":{"equal":30,"weight":1,"priority":1}},"variables":{"FertilizerA":{"N":36.00,"P":0.00,"K":0.00,"W":40},"FertilizerB":{"N":15.00,"P":15.00,"K":15.00,"W":40},"FertilizerC":{"N":5.00,"P":25.00,"K":10.00,"W":40}},"expects":{"feasible":true,"r1":46.66666667,"r4":10,"FertilizerA":0.83333333,"FertilizerB":1.33333333,"result":0}};



var solution = lpsolver.Solve(_obj1,1e-8,true,true);  //ƒ (model, precision, full, validate) so kann man mehr Infos sehen. 
var x= "hallo";