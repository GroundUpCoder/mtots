

## Associating Attributes to VBOs

1. Bind a VBO
  `gl.bindBuffer(gl.ARRAY_BUFFER, buffer)`
2. Point an attribute to the currently-bound VBO
  `gl.vertexAttribPointer(index, size, type, normalize, stride, offset)`
3. Enable the attribute
  `gl.enableVertexAttribArray(positionAttributeLocation)`
4. Unbind (cleanup)
  `gl.bindBuffer(gl.ARRAY_BUFFER, 0)`
