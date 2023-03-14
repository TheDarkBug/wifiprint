function getPageCount(buffer) {
  const dataView = new DataView(buffer);

  // Find the length of the binary string
  const length = dataView.byteLength;

  // Set up variables for reading the binary string in chunks
  const chunkSize = 4096;
  let binaryString = "";

  // Read the binary string in chunks and append to the final string
  for (let i = 0; i < length; i += chunkSize) {
    const chunk = new Uint8Array(dataView.buffer, i, Math.min(chunkSize, length - i));
    binaryString += String.fromCharCode.apply(null, chunk);
  }

  // Find the start and end positions of the /Page node using regular expressions
  const regex = /\/Type\s*\/Page[^s]/g;
  let pageCount = 0;
  while (regex.exec(binaryString) !== null) pageCount++;
  return pageCount;
}
