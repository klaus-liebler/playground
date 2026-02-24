import xml.etree.ElementTree as ET
from collections import defaultdict, Counter
import hashlib
import json

class OPCUANodeStructureAnalyzer:
    def __init__(self, nodeset_file):
        self.nodeset_file = nodeset_file
        self.tree = None
        self.root = None
        self.structure_patterns = defaultdict(list)
        
    def load_nodeset(self):
        """Lädt die NodeSet2 XML Datei"""
        self.tree = ET.parse(self.nodeset_file)
        self.root = self.tree.getroot()
        
    def normalize_node_id(self, node_id):
        """Normalisiert NodeIDs um absolute Werte zu entfernen"""
        if node_id.startswith('i='):
            return 'i=*'  # Numeric IDs normalisieren
        elif node_id.startswith('s='):
            return 's=*'  # String IDs normalisieren
        elif node_id.startswith('g='):
            return 'g=*'  # GUID IDs normalisieren
        return node_id
    
    def extract_node_structure(self, node_element):
        """Extrahiert die strukturellen Eigenschaften eines Nodes"""
        structure = {
            'node_class': node_element.tag,
            'browse_name': node_element.get('BrowseName', ''),
            'attributes': {},
            'references': []
        }
        
        # Attribute sammeln (ohne Werte, nur Typ-Information)
        for attr in node_element.attrib:
            if attr not in ['NodeId']:  # NodeId wird normalisiert
                structure['attributes'][attr] = node_element.get(attr, '')
        
        # Child-Elemente analysieren
        for child in node_element:
            if child.tag in ['DisplayName', 'Description']:
                # Textinhalt ignorieren, nur Struktur behalten
                structure['attributes'][child.tag] = f"{{{len(child.text) if child.text else 0}}}"
            elif child.tag == 'References':
                # Referenz-Struktur analysieren
                for ref in child:
                    ref_structure = {
                        'reference_type': self.normalize_node_id(ref.get('ReferenceType', '')),
                        'is_forward': ref.get('IsForward', 'true'),
                        'target_type': self.normalize_node_id(ref.text) if ref.text else ''
                    }
                    structure['references'].append(ref_structure)
            else:
                # Andere Elemente als Struktur-Marker behalten
                structure['attributes'][f"has_{child.tag}"] = True
        
        return structure
    
    def create_structure_fingerprint(self, structure):
        """Erstellt einen Fingerabdruck für eine Node-Struktur"""
        # Sortieren für konsistente Hashes
        sorted_attrs = json.dumps(structure['attributes'], sort_keys=True)
        sorted_refs = json.dumps(structure['references'], sort_keys=True)
        
        fingerprint_data = f"{structure['node_class']}:{sorted_attrs}:{sorted_refs}"
        return hashlib.md5(fingerprint_data.encode()).hexdigest()
    
    def analyze_structural_patterns(self):
        """Analysiert die NodeSet2 Datei auf strukturelle Muster"""
        print("Analysiere strukturelle Muster in OPC UA NodeSet2...")
        
        namespace = {'ua': 'http://opcfoundation.org/UA/2011/03/UANodeSet.xsd'}
        
        # Alle Node-Typen durchsuchen
        node_types = ['UAObject', 'UAVariable', 'UAMethod', 'UAObjectType', 
                     'UAVariableType', 'UADataType', 'UAReferenceType']
        
        for node_type in node_types:
            all_elements = self.root.findall(f'.//ua:{node_type}', namespace)
            elements = [
                elem for elem in all_elements 
                if elem.get('ReleaseStatus') != 'Deprecated'
            ]
            print(f"Analysiere {len(elements)} {node_type} Elemente...")
            
            for element in elements:
                structure = self.extract_node_structure(element)
                fingerprint = self.create_structure_fingerprint(structure)
                
                node_id = element.get('NodeId', 'Unknown')
                self.structure_patterns[fingerprint].append({
                    'node_id': node_id,
                    'structure': structure
                })
    
    def find_common_patterns(self, min_count=2):
        """Findet häufig auftretende Strukturmuster"""
        common_patterns = {}
        
        for fingerprint, nodes in self.structure_patterns.items():
            if len(nodes) >= min_count:
                common_patterns[fingerprint] = {
                    'count': len(nodes),
                    'sample_structure': nodes[0]['structure'],
                    'node_ids': [node['node_id'] for node in nodes]
                }
        
        return common_patterns
    
    def print_pattern_analysis(self, min_count=2):
        """Gibt eine Analyse der gefundenen Muster aus"""
        common_patterns = self.find_common_patterns(min_count)
        
        print(f"\n=== STRUKTURELLE MUSTER ANALYSIS ===")
        print(f"Gefunden {len(common_patterns)} verschiedene Strukturmuster")
        
        # Nach Häufigkeit sortieren
        sorted_patterns = sorted(common_patterns.items(), 
                               key=lambda x: x[1]['count'], reverse=True)
        
        for i, (fingerprint, pattern_data) in enumerate(sorted_patterns, 1):
            structure = pattern_data['sample_structure']
            print(f"\n--- Muster {i} (Häufigkeit: {pattern_data['count']}) ---")
            print(f"Node-Klasse: {structure['node_class']}")
            print(f"BrowseName: {structure['browse_name']}")
            print(f"Attribute: {len(structure['attributes'])}")
            print(f"Referenzen: {len(structure['references'])}")
            
            # Erste paar Node-IDs anzeigen
            sample_nodes = pattern_data['node_ids'][:3]
            print(f"Beispiel-Nodes: {sample_nodes}")
            
            if pattern_data['count'] > 3:
                print(f"... und {pattern_data['count'] - 3} weitere")
    
    def generate_template_code_suggestions(self):
        """Generiert Vorschläge für Code-Templates basierend auf den Mustern"""
        common_patterns = self.find_common_patterns(min_count=5)
        
        print(f"\n=== CODE TEMPLATE VORSCHLÄGE ===")
        
        for fingerprint, pattern_data in common_patterns.items():
            structure = pattern_data['sample_structure']
            
            print(f"\nTemplate für {structure['node_class']} (wird {pattern_data['count']}x verwendet):")
            
            # Generiere Template-Vorschlag basierend auf der Struktur
            if structure['node_class'] == 'UAVariable':
                self._generate_variable_template(structure)
            elif structure['node_class'] == 'UAObject':
                self._generate_object_template(structure)
            elif structure['node_class'] == 'UAMethod':
                self._generate_method_template(structure)
    
    def _generate_variable_template(self, structure):
        """Generiert ein Template für Variable Nodes"""
        print("""\
def create_variable_template(parent, browse_name, node_id, value=None):
    \"\"\"Erstellt eine Variable mit gemeinsamer Struktur\"\"\"
    from opcua import ua
    
    var = parent.add_variable(node_id, browse_name, value)
    # Gemeinsame Attribute setzen
    # var.set_attribute(attr_name, value)
    # Gemeinsame Referenzen hinzufügen
    return var""")
    
    def _generate_object_template(self, structure):
        """Generiert ein Template für Object Nodes"""
        print("""\
def create_object_template(parent, browse_name, node_id):
    \"\"\"Erstellt ein Object mit gemeinsamer Struktur\"\"\"
    from opcua import ua
    
    obj = parent.add_object(node_id, browse_name)
    # Gemeinsame Attribute setzen
    # Gemeinsame Referenzen hinzufügen
    return obj""")

def main():
    # Verwendung
    analyzer = OPCUANodeStructureAnalyzer('Opc.Ua.NodeSet2.xml')
    analyzer.load_nodeset()
    analyzer.analyze_structural_patterns()
    analyzer.print_pattern_analysis(min_count=3)
    analyzer.generate_template_code_suggestions()

if __name__ == "__main__":
    main()