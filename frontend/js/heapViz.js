function renderHeap(heap, zonesFromState) {
  // ── helpers ────────────────────────────────────────────────────────
  function fmt(name) {
    if (!name) return '?';
    return name.length <= 8 ? name : name.slice(0, 7) + '.';
  }

  // ── container ──────────────────────────────────────────────────────
  const container = document.getElementById('heapViz');
  if (!container) return;

  container.innerHTML = '';
  container.style.display  = 'flex';
  container.style.flexDirection = 'column';

  // ── zone name lookup — use argument first, fallback to window state ─
  const zones = zonesFromState
    || (window.__reliefState && window.__reliefState.zones)
    || [];
  const zoneLookup = new Map(zones.map(z => [z.id, z.name]));

  // ── empty state ────────────────────────────────────────────────────
  const nodes = Array.isArray(heap && heap.nodes)
    ? heap.nodes.map((node) => ({ ...node }))
    : [];
  if (!nodes.length) {
    const empty = document.createElement('div');
    empty.className = 'panel-empty';
    empty.textContent = 'Priority queue is empty';
    container.appendChild(empty);
    return;
  }

  // ── legend ─────────────────────────────────────────────────────────
  const legend = document.createElement('div');
  legend.style.cssText = [
    'flex:0 0 auto',
    'padding:0.3rem 1rem 0.4rem',
    'display:flex',
    'gap:1.2rem',
    'align-items:center',
    "font-family:'DM Mono',monospace",
    'font-size:10.5px',
    'color:#9a8e80',
    'border-bottom:1px solid rgba(60,45,30,0.1)',
    'background:rgba(245,240,232,0.5)',
  ].join(';');
  legend.innerHTML = `
    <span style="display:flex;align-items:center;gap:5px">
      <svg width="12" height="12"><circle cx="6" cy="6" r="6" fill="#c77b27"/></svg>
      Gold = Next to serve
    </span>
    <span style="display:flex;align-items:center;gap:5px">
      <svg width="12" height="12"><circle cx="6" cy="6" r="6" fill="#2d4a3e"/></svg>
      Waiting in queue
    </span>
    <span>Lower key = Higher priority</span>
  `;
  container.appendChild(legend);

  // ── canvas ─────────────────────────────────────────────────────────
  const canvas = document.createElement('div');
  canvas.style.cssText = 'flex:1 1 auto;min-height:0;overflow:hidden;';
  container.appendChild(canvas);

  // ── layout ─────────────────────────────────────────────────────────
  const W      = container.clientWidth  || 480;
  const H      = Math.max(container.clientHeight - 48, 140);
  const n      = nodes.length;
  const COLS   = 4;
  const rowCnt = Math.ceil(n / COLS);

  const NODE_R = Math.min(42, Math.floor(
    Math.min(W / (COLS * 2.7), H / (rowCnt * 2.7))
  ));
  const colGap = (W - NODE_R * 2) / Math.max(COLS - 1, 1);
  const rowGap = Math.min(130, (H - NODE_R * 2) / Math.max(rowCnt - 1, 1));

  const positioned = nodes.map((node, i) => ({
    ...node,
    x: NODE_R + (i % COLS) * colGap,
    y: NODE_R + Math.floor(i / COLS) * rowGap,
  }));

  const svgH = NODE_R + (rowCnt - 1) * rowGap + NODE_R + 10;

  // ── SVG ────────────────────────────────────────────────────────────
  const svg = d3.select(canvas)
    .append('svg')
    .attr('viewBox', `0 0 ${W} ${svgH}`)
    .attr('preserveAspectRatio', 'xMidYMid meet')
    .attr('width',  '100%')
    .attr('height', '100%');

  // ── defs ───────────────────────────────────────────────────────────
  const defs = svg.append('defs');

  const ng = defs.append('radialGradient')
    .attr('id','hv-ng').attr('cx','35%').attr('cy','35%').attr('r','65%');
  ng.append('stop').attr('offset','0%').attr('stop-color','#3d5c4a');
  ng.append('stop').attr('offset','100%').attr('stop-color','#1e3028');

  const mg = defs.append('radialGradient')
    .attr('id','hv-mg').attr('cx','35%').attr('cy','35%').attr('r','65%');
  mg.append('stop').attr('offset','0%').attr('stop-color','#e8a84a');
  mg.append('stop').attr('offset','100%').attr('stop-color','#b86e1a');

  const flt = defs.append('filter')
    .attr('id','hv-glow')
    .attr('x','-40%').attr('y','-40%')
    .attr('width','180%').attr('height','180%');
  flt.append('feGaussianBlur').attr('stdDeviation','3').attr('result','blur');
  const fm = flt.append('feMerge');
  fm.append('feMergeNode').attr('in','blur');
  fm.append('feMergeNode').attr('in','SourceGraphic');

  // ── edges ──────────────────────────────────────────────────────────
  svg.append('g')
    .selectAll('line')
    .data(positioned.slice(1))
    .enter()
    .append('line')
    .attr('x1', (_, i) => positioned[Math.floor(i / 2)].x)
    .attr('y1', (_, i) => positioned[Math.floor(i / 2)].y)
    .attr('x2', d => d.x)
    .attr('y2', d => d.y)
    .attr('stroke', 'rgba(60,45,30,0.2)')
    .attr('stroke-width', 1.5)
    .attr('stroke-dasharray', '4 3');

  // ── node groups ────────────────────────────────────────────────────
  const g = svg.append('g')
    .selectAll('g')
    .data(positioned)
    .enter()
    .append('g')
    .attr('transform', d => `translate(${d.x},${d.y})`);

  const isMin = d => d.id === heap.min_id;

  // outer dashed ring on min node
  g.filter(isMin)
    .append('circle')
    .attr('r', NODE_R + 5)
    .attr('fill', 'none')
    .attr('stroke', '#c77b27')
    .attr('stroke-width', 1.5)
    .attr('stroke-dasharray', '3 3')
    .attr('opacity', 0.6);

  // main circle
  g.append('circle')
    .attr('r', NODE_R)
    .attr('fill',   d => isMin(d) ? 'url(#hv-mg)' : 'url(#hv-ng)')
    .attr('filter', d => isMin(d) ? 'url(#hv-glow)' : null)
    .attr('stroke', d => isMin(d) ? '#e8a84a' : 'rgba(255,255,255,0.12)')
    .attr('stroke-width', d => isMin(d) ? 2 : 1)
    .attr('class',  d => isMin(d) ? 'heap-min-pulse' : null);

  const FS_NAME = Math.max(9,  NODE_R * 0.27);
  const FS_KEY  = Math.max(8,  NODE_R * 0.23);
  const FS_NXT  = Math.max(7,  NODE_R * 0.20);

  // zone name — line 1
  g.append('text')
    .attr('text-anchor', 'middle')
    .attr('dy', d => isMin(d) ? '-1.15em' : '-0.45em')
    .attr('fill', '#fff')
    .attr('font-family', "'DM Sans',sans-serif")
    .attr('font-size', FS_NAME)
    .attr('font-weight', '600')
    .text(d => fmt(zoneLookup.get(d.id) || `Z${d.id}`));

  // key — line 2
  g.append('text')
    .attr('text-anchor', 'middle')
    .attr('dy', d => isMin(d) ? '0.25em' : '0.85em')
    .attr('fill', d => isMin(d) ? '#fde8c0' : 'rgba(255,255,255,0.65)')
    .attr('font-family', "'DM Mono',monospace")
    .attr('font-size', FS_KEY)
    .text(d => Math.round(d.key));

  // NEXT label — line 3 (min node only)
  g.filter(isMin)
    .append('text')
    .attr('text-anchor', 'middle')
    .attr('dy', '1.55em')
    .attr('fill', '#fde8c0')
    .attr('font-family', "'DM Mono',monospace")
    .attr('font-size', FS_NXT)
    .attr('font-weight', '700')
    .attr('letter-spacing', '0.05em')
    .text('⚡ NEXT');
}
