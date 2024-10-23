import * as fs from 'fs';
import * as path from 'path';

const filePath = path.join(__dirname, 'index.js');
const shebang = '#!/usr/bin/env node\n';

fs.readFile(filePath, 'utf8', (err, data) => {
  if (err) {
    console.error('Error reading file:', err);
    process.exit(1);
  }

  if (data.startsWith(shebang)) {
    console.log('Shebang already exists.');
    process.exit(0);
  }

  const updatedData = shebang + data;

  fs.writeFile(filePath, updatedData, 'utf8', (err) => {
    if (err) {
      console.error('Error writing file:', err);
      process.exit(1);
    }
    console.log('Build generated successfully.');
  });
});