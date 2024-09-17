let currentEditItem = null;
let stepElements = [];

function addStep() {
    const stepType = document.getElementById('stepType').value;
    const stepValue = document.getElementById('stepValue').value;
    if (stepType && stepValue) {
        // Create step object
        const step = {
            type: stepType,
            value: stepValue
        };
        stepElements.push(step);
        updateSvg();

        // Clear input fields
        document.getElementById('stepType').value = "Motor schnell";
        document.getElementById('stepValue').value = "";
    } else {
        alert('Bitte einen Wert und Typ eingeben');
    }
}

function editStep(index) {
    const step = stepElements[index];
    currentEditItem = index;
    document.getElementById('stepType').value = step.type;
    document.getElementById('stepValue').value = step.value;
    document.getElementById('addButton').style.display = 'none';
    document.getElementById('updateButton').style.display = 'block';
}

function updateStep() {
    if (currentEditItem !== null) {
        const stepType = document.getElementById('stepType').value;
        const stepValue = document.getElementById('stepValue').value;
        stepElements[currentEditItem] = {
            type: stepType,
            value: stepValue
        };
        currentEditItem = null;
        updateSvg();
        document.getElementById('addButton').style.display = 'block';
        document.getElementById('updateButton').style.display = 'none';
        document.getElementById('stepType').value = "Motor schnell";
        document.getElementById('stepValue').value = "";
    }
}

function deleteStep(index) {
    stepElements.splice(index, 1);
    updateSvg();
}

function moveStep(index, direction) {
    const newIndex = index + direction;
    if (newIndex >= 0 && newIndex < stepElements.length) {
        const temp = stepElements[index];
        stepElements[index] = stepElements[newIndex];
        stepElements[newIndex] = temp;
        updateSvg();
    }
}

function createSvgElement(name, attributes) {
    const element = document.createElementNS("http://www.w3.org/2000/svg", name);
    for (const attr in attributes) {
        element.setAttribute(attr, attributes[attr]);
    }
    return element;
}

function updateSvg() {
    const svg = document.getElementById('stepsSvg');
    svg.innerHTML = ''; // Clear existing SVG content

    stepElements.forEach((step, index) => {
        const x = 50;
        const y = 50 + index * 100;
        const width = 200;
        const height = 60;

        // Create rounded rectangle
        const rect = createSvgElement('rect', {
            x, y, width, height,
            rx: 15, ry: 15,
            fill: '#f8f9fa',
            stroke: '#ddd',
            'stroke-width': 2
        });
        svg.appendChild(rect);

        // Create text for step type
        const stepTypeText = createSvgElement('text', {
            x: x + 10,
            y: y + 25,
            'font-size': '14px',
            'font-family': 'Arial'
        });
        stepTypeText.textContent = step.type;
        svg.appendChild(stepTypeText);

        // Create text for step value
        const stepValueText = createSvgElement('text', {
            x: x + 10,
            y: y + 45,
            'font-size': '14px',
            'font-family': 'Arial'
        });
        stepValueText.textContent = `Wert: ${step.value}`;
        svg.appendChild(stepValueText);

        // Create edit button
        const editButton = createSvgElement('text', {
            x: x + 150,
            y: y + 25,
            'font-size': '12px',
            'font-family': 'Arial',
            'fill': 'blue',
            'cursor': 'pointer'
        });
        editButton.textContent = 'Bearbeiten';
        editButton.addEventListener('click', () => editStep(index));
        svg.appendChild(editButton);

        // Create delete button
        const deleteButton = createSvgElement('text', {
            x: x + 150,
            y: y + 45,
            'font-size': '12px',
            'font-family': 'Arial',
            'fill': 'red',
            'cursor': 'pointer'
        });
        deleteButton.textContent = 'Löschen';
        deleteButton.addEventListener('click', () => deleteStep(index));
        svg.appendChild(deleteButton);

        // Create move up button
        if (index > 0) {
            const moveUpButton = createSvgElement('text', {
                x: x + 150,
                y: y + 5,
                'font-size': '12px',
                'font-family': 'Arial',
                'fill': 'green',
                'cursor': 'pointer'
            });
            moveUpButton.textContent = '↑';
            moveUpButton.addEventListener('click', () => moveStep(index, -1));
            svg.appendChild(moveUpButton);
        }

        

        // Create move down button
        if (index < stepElements.length - 1) {
            const moveDownButton = createSvgElement('text', {
                x: x + 170,
                y: y + 5,
                'font-size': '12px',
                'font-family': 'Arial',
                'fill': 'green',
                'cursor': 'pointer'
            });
            moveDownButton.textContent = '↓';
            moveDownButton.addEventListener('click', () => moveStep(index, 1));
            svg.appendChild(moveDownButton);
        }

        // Draw arrow to next step
        if (index < stepElements.length - 1) {
            const arrow = createSvgElement('path', {
                d: `M${x + width / 2} ${y + height} L${x + width / 2} ${y + height + 40} L${x + width / 2 - 10} ${y + height + 30} M${x + width / 2} ${y + height + 40} L${x + width / 2 + 10} ${y + height + 30}`,
                stroke: 'black',
                'stroke-width': 2,
                fill: 'none'
            });
            svg.appendChild(arrow);
        }
    });

    // Update SVG canvas size to fit all steps
    svg.setAttribute('height', (stepElements.length * 100 + 50) + 'px');
}