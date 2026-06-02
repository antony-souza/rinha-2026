import http from 'k6/http';
import { check } from 'k6';

export const options = {
  scenarios: {
    known_ids: {
      executor: 'constant-vus',
      vus: Number(__ENV.VUS || 100),
      duration: __ENV.DURATION || '15s',
    },
  },
  summaryTrendStats: ['avg', 'min', 'med', 'max', 'p(90)', 'p(95)', 'p(99)'],
  thresholds: {
    http_req_failed: ['rate==0'],
  },
};

const ids = [100398, 241092, 326580, 361348, 438894, 693008, 841023, 844584];
const url = __ENV.API_URL || 'http://host.docker.internal:9999/fraud-score';
const headers = { 'Content-Type': 'application/json' };

export default function () {
  const id = ids[(__ITER + __VU) % ids.length];
  const body = JSON.stringify({
    id: `tx-${id}`,
    transaction: { amount: 384.88, installments: 3, requested_at: '2026-03-11T20:23:35Z' },
    customer: { avg_amount: 769.76, tx_count_24h: 3, known_merchants: ['MERC-009', 'MERC-001'] },
    merchant: { id: 'MERC-001', mcc: '5912', avg_amount: 298.95 },
    terminal: { is_online: false, card_present: true, km_from_home: 13.7 },
    last_transaction: { timestamp: '2026-03-11T14:58:35Z', km_from_current: 18.8 },
  });
  const response = http.post(url, body, { headers });
  check(response, { 'status 200': (result) => result.status === 200 });
}
