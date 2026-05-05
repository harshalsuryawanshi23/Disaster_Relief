function normalizeTrieRows(triePath) {
  if (!Array.isArray(triePath) || !triePath.length) return [];
  if (Array.isArray(triePath[0]))
    return triePath.map(row => row.map(String)).filter(r => r.length);
  if (triePath.every(p => typeof p === 'string' && p.length === 1))
    return [triePath.map(String)];
  return triePath.filter(w => typeof w === 'string' && w.length).map(w => w.split(''));
}

function sharedPrefixLength(rows) {
  if (rows.length < 2) return 0;
  const shortest = Math.min(...rows.map(r => r.length));
  let len = 0;
  while (len < shortest && rows.every(r => r[len] === rows[0][len])) len++;
  return len;
}

// Auto-detect other zone names matching the same prefix
function enrichWithKnownMatches(rows) {
  if (rows.length !== 1) return rows;
  const zones = (window.__reliefState && window.__reliefState.zones) || [];
  const prefix = rows[0].join('').toLowerCase();
  const currentWord = rows[0].join('');
  const extras = zones
    .map(z => z.name)
    .filter(name => name.toLowerCase().startsWith(prefix) && name !== currentWord)
    .map(name => name.split(''));
  return extras.length ? [rows[0], ...extras] : rows;
}

function renderTriePath(triePath) {
  const container = document.getElementById('trieViz');
  if (!container) return;
  container.innerHTML = '';

  let rows = normalizeTrieRows(triePath);
  if (!rows.length) {
    container.innerHTML = '<div class="panel-empty">Type "search Na" to see trie traversal</div>';
    return;
  }

  rows = enrichWithKnownMatches(rows);

  const prefixLen = sharedPrefixLength(rows);
  const rowCount  = rows.length;
  const maxLength = Math.max(...rows.map(r => r.length));

  const W      = container.clientWidth  || 420;
  const H      = container.clientHeight || 200;
  const PAD    = 22;
  const labelW = rowCount > 1 ? 90 : 0;
  const usable = Math.max(80, W - PAD * 2 - labelW);
  const spacing= maxLength > 1 ? usable / (maxLength - 1) : 0;
  const R      = Math.min(19, Math.max(12, spacing * 0.24));
  const rowH   = Math.max(50, Math.min(76, (H - PAD * 2) / Math.max(rowCount, 1)));
  const annotH = rowCount > 1 ? 20 : 0;
  const totalH = PAD + rowCount * rowH + annotH + PAD * 0.5;

  const svg = d3.select(container)
    .append('svg')
    .attr('viewBox', `0 0 ${W} ${totalH}`)
    .attr('preserveAspectRatio', 'xMidYMid meet')
    .attr('width',  '100%')
    .attr('height', '100%');

  const defs = svg.append('defs');
  function radGrad(id, c1, c2) {
    const g = defs.append('radialGradient').attr('id', id).attr('cx','35%').attr('cy','35%').attr('r','65%');
    g.append('stop').attr('offset','0%').attr('stop-color', c1);
    g.append('stop').attr('offset','100%').attr('stop-color', c2);
  }
  radGrad('tv-node',   '#3d5c4a', '#1e3028');
  radGrad('tv-shared', '#c77b27', '#8a5210');
  radGrad('tv-found',  '#52b788', '#2d6a4f');

  const gf = defs.append('filter').attr('id','tv-glow').attr('x','-50%').attr('y','-50%').attr('width','200%').attr('height','200%');
  gf.append('feGaussianBlur').attr('stdDeviation','3').attr('result','blur');
  const fm = gf.append('feMerge');
  fm.append('feMergeNode').attr('in','blur');
  fm.append('feMergeNode').attr('in','SourceGraphic');

  // Compute y positions
  const rowY = ri => PAD + ri * rowH + rowH * 0.5;
  const nodeX = ci => PAD + ci * spacing;

  // ── Draw branch lines from shared prefix divergence ──────────────
  if (rowCount > 1 && prefixLen > 0) {
    const branchX = nodeX(prefixLen - 1);
    rows.forEach((row, ri) => {
      if (ri === 0) return;
      // line from shared prefix last node down to this row's first unique node
      const y0 = rowY(0);
      const y1 = rowY(ri);
      const x1 = nodeX(prefixLen);
      svg.append('line')
        .attr('x1', branchX).attr('y1', y0)
        .attr('x2', x1).attr('y2', y1)
        .attr('stroke', 'rgba(60,120,80,0.35)')
        .attr('stroke-width', 1.5)
        .attr('stroke-dasharray', '4 3');
    });
  }

  // ── Draw each row ─────────────────────────────────────────────────
  rows.forEach((row, ri) => {
    const y = rowY(ri);

    // horizontal connector lines
    row.slice(0, -1).forEach((_, ci) => {
      // skip drawing shared prefix connectors for rows > 0
      if (ri > 0 && ci < prefixLen) return;
      const isShared = ri === 0 && ci < prefixLen - 1;
      svg.append('line')
        .attr('x1', nodeX(ci)).attr('y1', y)
        .attr('x2', nodeX(ci + 1)).attr('y2', y)
        .attr('stroke', isShared ? '#c77b27' : 'rgba(60,130,80,0.45)')
        .attr('stroke-width', isShared ? 3 : 2)
        .attr('stroke-linecap', 'round');
    });

    // circle nodes
    row.forEach((char, ci) => {
      // rows > 0: skip rendering shared prefix nodes (row 0 has them)
      if (ri > 0 && ci < prefixLen) return;

      const x       = nodeX(ci);
      const isLast   = ci === row.length - 1;
      const isShared = ri === 0 && ci < prefixLen && !isLast;
      const fill     = isLast ? 'url(#tv-found)' : isShared ? 'url(#tv-shared)' : 'url(#tv-node)';
      const stroke   = isLast ? '#52b788' : isShared ? '#c77b27' : 'rgba(255,255,255,0.12)';

      if (isLast) {
        svg.append('circle')
          .attr('cx', x).attr('cy', y).attr('r', R + 5)
          .attr('fill', 'none')
          .attr('stroke', '#52b788')
          .attr('stroke-width', 1.5)
          .attr('stroke-dasharray', '3 3')
          .attr('opacity', 0.5);
      }

      svg.append('circle')
        .attr('cx', x).attr('cy', y).attr('r', R)
        .attr('fill', fill)
        .attr('stroke', stroke)
        .attr('stroke-width', isLast || isShared ? 1.5 : 1)
        .attr('filter', isLast ? 'url(#tv-glow)' : null);

      svg.append('text')
        .attr('x', x).attr('y', y)
        .attr('text-anchor', 'middle')
        .attr('dominant-baseline', 'central')
        .attr('fill', '#fff')
        .attr('font-family', "'DM Sans', 'Inter', sans-serif")
        .attr('font-size', R >= 16 ? '12px' : '10px')
        .attr('font-weight', '600')
        .text(char);
    });

    // row label — zone name after last node
    if (rowCount > 1) {
      svg.append('text')
        .attr('x', nodeX(row.length - 1) + R + 9)
        .attr('y', y)
        .attr('dominant-baseline', 'central')
        .attr('fill', '#9a8e80')
        .attr('font-family', "'DM Mono', monospace")
        .attr('font-size', '9.5px')
        .text(row.join(''));
    }
  });

  // ── Shared prefix annotation underline ───────────────────────────
  if (rowCount > 1 && prefixLen > 0) {
    const bY  = PAD + rowCount * rowH + 6;
    const bX1 = PAD - R;
    const bX2 = nodeX(prefixLen - 1) + R;
    svg.append('line')
      .attr('x1', bX1).attr('y1', bY)
      .attr('x2', bX2).attr('y2', bY)
      .attr('stroke', '#c77b27').attr('stroke-width', 1.5).attr('opacity', 0.7);
    svg.append('text')
      .attr('x', (bX1 + bX2) / 2).attr('y', bY + 10)
      .attr('text-anchor', 'middle')
      .attr('fill', '#c77b27')
      .attr('font-family', "'DM Mono', monospace")
      .attr('font-size', '8.5px')
      .attr('letter-spacing', '0.1em')
      .text('shared prefix');
  }
}