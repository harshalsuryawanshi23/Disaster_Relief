function renderHeap(heap) {
  const container = document.getElementById('heapViz');
  if (!container) {
    return;
  }

  container.innerHTML = '';
  const width = container.clientWidth || 420;
  const height = container.clientHeight || 260;

  if (!heap.nodes || !heap.nodes.length) {
    container.innerHTML = '<div class="panel-empty">Priority queue is empty</div>';
    return;
  }

  const svg = d3
    .select(container)
    .append('svg')
    .attr('viewBox', `0 0 ${width} ${height}`)
    .attr('width', '100%')
    .attr('height', '100%');

  const nodes = heap.nodes.map((node, index) => ({
    ...node,
    x: 90 + (index % 4) * ((width - 140) / 3),
    y: 70 + Math.floor(index / 4) * 90
  }));

  svg
    .append('g')
    .selectAll('line')
    .data(nodes.slice(1))
    .enter()
    .append('line')
    .attr('x1', (_, index) => nodes[Math.floor(index / 2)].x)
    .attr('y1', (_, index) => nodes[Math.floor(index / 2)].y)
    .attr('x2', (d) => d.x)
    .attr('y2', (d) => d.y)
    .attr('stroke', '#cdbba4')
    .attr('stroke-width', 2);

  const groups = svg
    .append('g')
    .selectAll('g')
    .data(nodes)
    .enter()
    .append('g')
    .attr('transform', (d) => `translate(${d.x}, ${d.y})`);

  groups
    .append('circle')
    .attr('r', 26)
    .attr('fill', (d) => (d.id === heap.min_id ? '#c6a44a' : '#1d2a22'))
    .attr('stroke', '#fffaf3')
    .attr('stroke-width', 3);

  groups
    .append('text')
    .attr('text-anchor', 'middle')
    .attr('dy', '-0.15em')
    .attr('fill', '#fffaf3')
    .style('font-family', '"IBM Plex Mono", monospace')
    .style('font-size', '12px')
    .text((d) => `Z${d.id}`);

  groups
    .append('text')
    .attr('text-anchor', 'middle')
    .attr('dy', '1.25em')
    .attr('fill', '#f5d7c8')
    .style('font-family', '"IBM Plex Mono", monospace')
    .style('font-size', '10px')
    .text((d) => `${Math.round(d.key)}`);
}
