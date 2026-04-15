function normalizeTrieRows(triePath) {
  if (!Array.isArray(triePath) || !triePath.length) {
    return [];
  }

  if (Array.isArray(triePath[0])) {
    return triePath
      .map((row) => (Array.isArray(row) ? row.map((part) => String(part)) : []))
      .filter((row) => row.length);
  }

  if (triePath.every((part) => typeof part === 'string' && part.length === 1)) {
    return [triePath.map((part) => String(part))];
  }

  return triePath
    .filter((word) => typeof word === 'string' && word.length)
    .map((word) => word.split(''));
}

function sharedPrefixLength(rows) {
  if (rows.length < 2) {
    return 0;
  }

  const shortest = Math.min(...rows.map((row) => row.length));
  let prefixLength = 0;

  while (prefixLength < shortest) {
    const expected = rows[0][prefixLength];
    if (rows.every((row) => row[prefixLength] === expected)) {
      prefixLength += 1;
    } else {
      break;
    }
  }

  return prefixLength;
}

function renderTriePath(triePath) {
  const container = document.getElementById('trieViz');
  if (!container) {
    return;
  }

  container.innerHTML = '';

  const rows = normalizeTrieRows(triePath);
  if (!rows.length) {
    container.innerHTML = '<div class="panel-empty">No active trie traversal</div>';
    return;
  }

  const width = container.clientWidth || 420;
  const rowHeight = 82;
  const height = Math.max(120, rows.length * rowHeight + 24);
  const maxLength = Math.max(...rows.map((row) => row.length));
  const prefixLength = sharedPrefixLength(rows);
  const startX = 34;
  const labelSpace = rows.length > 1 ? 120 : 36;
  const usableWidth = Math.max(120, width - startX * 2 - labelSpace);
  const spacing = maxLength > 1 ? usableWidth / (maxLength - 1) : 0;
  const radius = Math.min(18, Math.max(14, spacing * 0.24 || 18));

  const svg = d3
    .select(container)
    .append('svg')
    .attr('viewBox', `0 0 ${width} ${height}`)
    .attr('width', '100%')
    .attr('height', '100%');

  const layer = svg.append('g').attr('transform', 'translate(0,12)');

  rows.forEach((row, rowIndex) => {
    const y = 28 + rowIndex * rowHeight;
    const rowGroup = layer.append('g').attr('transform', `translate(0,${y})`);

    if (row.length > 1) {
      row.slice(0, -1).forEach((_, charIndex) => {
        const isSharedSegment = rowIndex === 0 && charIndex < prefixLength - 1;
        rowGroup
          .append('line')
          .attr('x1', startX + charIndex * spacing)
          .attr('y1', 0)
          .attr('x2', startX + (charIndex + 1) * spacing)
          .attr('y2', 0)
          .attr('stroke', isSharedSegment ? '#bf5c2b' : '#d3c2ae')
          .attr('stroke-width', isSharedSegment ? 4 : 3)
          .attr('stroke-linecap', 'round');
      });
    }

    row.forEach((char, charIndex) => {
      const x = startX + charIndex * spacing;
      const isLast = charIndex === row.length - 1;
      const isSharedPrefix = rowIndex === 0 && charIndex < prefixLength && !isLast;

      rowGroup
        .append('circle')
        .attr('cx', x)
        .attr('cy', 0)
        .attr('r', radius)
        .attr('fill', isLast ? '#2d7b59' : isSharedPrefix ? '#bf5c2b' : '#1d2a22');

      rowGroup
        .append('text')
        .attr('x', x)
        .attr('y', 0)
        .attr('text-anchor', 'middle')
        .attr('dy', '0.35em')
        .attr('fill', '#fffaf3')
        .style('font-family', '"Space Grotesk", sans-serif')
        .style('font-size', radius >= 17 ? '14px' : '12px')
        .text(char);
    });

    if (rows.length > 1) {
      rowGroup
        .append('text')
        .attr('x', startX + (row.length - 1) * spacing + radius + 14)
        .attr('y', 0)
        .attr('dy', '0.35em')
        .attr('fill', '#6f7166')
        .style('font-family', '"IBM Plex Mono", monospace')
        .style('font-size', '12px')
        .text(`(${row.join('')})`);
    }
  });
}
