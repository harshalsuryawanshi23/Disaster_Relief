let disasterMap;
let zoneLayer;
let depotLayer;
let routeLayer;

function ensureMap() {
  if (disasterMap) {
    return disasterMap;
  }

  disasterMap = L.map('map', {
    zoomControl: false
  }).setView([19.4, 75.1], 6);

  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; OpenStreetMap contributors'
  }).addTo(disasterMap);

  L.control.zoom({ position: 'bottomright' }).addTo(disasterMap);
  zoneLayer = L.layerGroup().addTo(disasterMap);
  depotLayer = L.layerGroup().addTo(disasterMap);
  routeLayer = L.layerGroup().addTo(disasterMap);
  return disasterMap;
}

function zoneColor(zone) {
  if (zone.is_served) {
    return '#2d7b59';
  }
  if (zone.severity >= 8) {
    return '#be3d30';
  }
  if (zone.severity >= 6) {
    return '#cf4336';
  }
  if (zone.severity >= 4) {
    return '#dc5648';
  }
  return '#e77a70';
}

function renderMapState(state) {
  window.__reliefState = state;
  const map = ensureMap();
  const zones = state.zones || [];
  const depots = state.depots || [];

  zoneLayer.clearLayers();
  depotLayer.clearLayers();
  routeLayer.clearLayers();

  const bounds = [];
  zones.forEach((zone) => {
    const marker = L.circleMarker([zone.lat, zone.lng], {
      radius: 10,
      color: '#ffffff',
      weight: 2,
      fillColor: zoneColor(zone),
      fillOpacity: 0.92
    });
    marker.bindPopup(
      `<strong>${zone.name}</strong><br>Severity: ${zone.severity}<br>Served: ${zone.is_served}<br>Depot: ${zone.assigned_depot}`
    );
    marker.addTo(zoneLayer);
    bounds.push([zone.lat, zone.lng]);
  });

  depots.forEach((depot) => {
    const marker = L.marker([depot.lat, depot.lng], {
      title: depot.name
    });
    marker.bindPopup(
      `<strong>${depot.name}</strong><br>Food: ${depot.food_stock}<br>Medicine: ${depot.medicine_stock}<br>Active: ${depot.is_active}`
    );
    marker.addTo(depotLayer);
    bounds.push([depot.lat, depot.lng]);
  });

  const lastDispatch = (state.dispatch_log || []).slice(-1)[0];
  if (lastDispatch && Array.isArray(lastDispatch.route)) {
    const zoneLookup = new Map(zones.map((zone) => [zone.id, zone]));
    const polylinePoints = lastDispatch.route
      .map((zoneId) => zoneLookup.get(zoneId))
      .filter(Boolean)
      .map((zone) => [zone.lat, zone.lng]);

    if (polylinePoints.length > 1) {
      L.polyline(polylinePoints, {
        color: '#1d2a22',
        dashArray: '10 8',
        weight: 4,
        opacity: 0.8
      }).addTo(routeLayer);
    }
  }

  if (bounds.length) {
    map.fitBounds(bounds, { padding: [32, 32] });
  }
}

function renderDispatchLog(dispatchLog, zones, depots) {
  const container = document.getElementById('dispatchLog');
  if (!container) {
    return;
  }

  const zoneLookup = new Map((zones || []).map((zone) => [zone.id, zone.name]));
  const depotLookup = new Map((depots || []).map((depot) => [depot.id, depot.name]));

  if (!dispatchLog.length) {
    container.innerHTML = '<div class="panel-empty">No dispatches yet</div>';
    return;
  }

  container.innerHTML = dispatchLog
    .slice()
    .reverse()
    .map((entry) => `
      <article class="log-card">
        <h3>Cycle ${entry.cycle}: ${zoneLookup.get(entry.zone_id) || `Zone ${entry.zone_id}`}</h3>
        <p>Served by ${depotLookup.get(entry.depot_id) || `Depot ${entry.depot_id}`}</p>
        <p>Route: ${(entry.route || []).join(' -> ')}</p>
        <p>Food ${entry.food}, Medicine ${entry.medicine}</p>
        <span class="metric-chip">${Number(entry.distance_km || 0).toFixed(1)} km</span>
      </article>
    `)
    .join('');
}
