function renderSkipList(skipList) {
  const container = document.getElementById('skipListViz');
  if (!container) return;
  container.innerHTML = '';

  const levels = skipList.levels || [];
  if (!levels.length) {
    container.innerHTML = '<div class="panel-empty">Skip list has no nodes</div>';
    return;
  }

  const W = container.clientWidth  || 420;
  const H = container.clientHeight || 220;

  const PAD      = 20;
  const laneCount = levels.length;
  const laneH    = Math.max(52, (H - PAD * 2) / Math.max(laneCount + 0.5, 2));
  const totalH   = PAD * 2 + laneCount * laneH;

  const svg = d3.select(container)
    .append('svg')
    .attr('viewBox', `0 0 ${W} ${totalH}`)
    .attr('preserveAspectRatio', 'xMidYMid meet')
    .attr('width', '100%')
    .attr('height', '100%');

  const defs = svg.append('defs');
  const g1 = defs.append('linearGradient').attr('id','sl-node').attr('x1','0%').attr('y1','0%').attr('x2','0%').attr('y2','100%');
  g1.append('stop').attr('offset','0%').attr('stop-color','#f5f0e8');
  g1.append('stop').attr('offset','100%').attr('stop-color','#e8e0d4');

  levels.forEach((levelNodes, li) => {
    const y   = PAD + li * laneH;
    const lvl = laneCount - li - 1;

    // lane label
    svg.append('text')
      .attr('x', PAD).attr('y', y - 10)
      .attr('fill', '#9a8e80')
      .attr('font-family', "'DM Mono', monospace")
      .attr('font-size', '10px')
      .attr('letter-spacing', '0.1em')
      .text(`L${lvl}`);

    // lane line
    svg.append('line')
      .attr('x1', PAD + 28).attr('x2', W - PAD).attr('y1', y).attr('y2', y)
      .attr('stroke', 'rgba(60,45,30,0.15)')
      .attr('stroke-width', 1.5)
      .attr('stroke-dasharray', '6 5');

    // HEAD sentinel
    svg.append('rect')
      .attr('x', PAD + 2).attr('y', y - 14).attr('width', 24).attr('height', 28).attr('rx', 5)
      .attr('fill', '#2d6a4f').attr('stroke', 'none');
    svg.append('text')
      .attr('x', PAD + 14).attr('y', y)
      .attr('text-anchor', 'middle').attr('dominant-baseline', 'central')
      .attr('fill', '#fff')
      .attr('font-family', "'DM Mono', monospace").attr('font-size', '7px').attr('font-weight', '700')
      .text('H');

    const maxNodes  = Math.max(...levels.map(l => l.length));
    const nodeW     = 56;
    const nodeH     = 30;
    const slotW     = Math.max(nodeW + 10, (W - PAD * 2 - 50) / Math.max(maxNodes, 1));

    levelNodes.forEach((node, ni) => {
      const x = PAD + 50 + ni * slotW;

      // forward arrow
      const nextX = ni < levelNodes.length - 1 ? x + nodeW + (slotW - nodeW) * 0.5 : x + nodeW + 8;
      svg.append('line')
        .attr('x1', x + nodeW).attr('y1', y)
        .attr('x2', nextX - 4).attr('y2', y)
        .attr('stroke', 'rgba(45,106,79,0.35)')
        .attr('stroke-width', 1.5)
        .attr('stroke-dasharray', li === 0 ? 'none' : '4 3')
        .attr('marker-end', 'none');

      // node box
      svg.append('rect')
        .attr('x', x).attr('y', y - nodeH / 2)
        .attr('width', nodeW).attr('height', nodeH)
        .attr('rx', 8)
        .attr('fill', 'url(#sl-node)')
        .attr('stroke', '#52b788')
        .attr('stroke-width', 1);

      // key
      svg.append('text')
        .attr('x', x + nodeW / 2).attr('y', y - 3)
        .attr('text-anchor', 'middle').attr('dominant-baseline', 'central')
        .attr('fill', '#1c1712')
        .attr('font-family', "'DM Mono', monospace").attr('font-size', '11px').attr('font-weight', '500')
        .text(Math.round(node.key));

      // depot id
      svg.append('text')
        .attr('x', x + nodeW / 2).attr('y', y + 10)
        .attr('text-anchor', 'middle').attr('dominant-baseline', 'central')
        .attr('fill', '#c77b27')
        .attr('font-family', "'DM Mono', monospace").attr('font-size', '9px').attr('font-weight', '500')
        .text(`D${node.depot_id}`);
    });
  });
}