function renderUnionFind(unionFind, zones) {
  const container = document.getElementById('unionFindViz');
  if (!container) {
    return;
  }

  container.innerHTML = '';
  const components = unionFind.components || [];
  if (!components.length) {
    container.innerHTML = '<div class="panel-empty">No components available</div>';
    return;
  }

  const zoneLookup = new Map((zones || []).map((zone) => [zone.id, zone.name]));
  const palette = ['#1d2a22', '#2d7b59', '#bf5c2b', '#c6a44a', '#7f8f6a'];

  const wrapper = document.createElement('div');
  wrapper.style.display = 'grid';
  wrapper.style.gridTemplateColumns = 'repeat(auto-fit, minmax(180px, 1fr))';
  wrapper.style.gap = '0.8rem';
  wrapper.style.padding = '0 1rem 1rem';

  components.forEach((component, index) => {
    const card = document.createElement('article');
    card.className = 'log-card';
    card.style.background = 'rgba(255, 250, 243, 0.94)';
    card.innerHTML = `
      <h3 style="color:${palette[index % palette.length]}">Component ${index + 1}</h3>
      <p>${component
        .map((zoneId) => zoneLookup.get(zoneId) || `Zone ${zoneId}`)
        .join(', ')}</p>
    `;
    wrapper.appendChild(card);
  });

  container.appendChild(wrapper);
}
