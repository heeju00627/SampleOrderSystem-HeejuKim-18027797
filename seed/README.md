# 시드 데이터

`data/` 폴더에 복사해서 사용한다.

```powershell
Copy-Item seed\samples.json data\samples.json
Copy-Item seed\orders.json  data\orders.json
echo '{"seq":5}' | Out-File data\order_seq.json -Encoding utf8
```

## 시료 (8종)

| ID | 이름 | min/ea | 수율 | 재고 |
|----|------|--------|------|------|
| S0001 | GaN 버퍼 | 1.2 | 0.88 | 150 |
| S0002 | AlGaN 에피 | 1.8 | 0.82 | 80 |
| S0003 | SiC 기판 | 0.6 | 0.95 | 200 |
| S0004 | InGaN 양자우물 | 2.0 | 0.75 | 30 |
| S0005 | GaAs 기판 | 0.8 | 0.92 | 0 (고갈) |
| S0006 | InP 에피 | 1.5 | 0.80 | 55 |
| S0007 | ZnO 박막 | 0.4 | 0.90 | 300 |
| S0008 | Si 도핑층 | 0.3 | 0.97 | 500 |

## 주문 (5건)

| ID | 시료 | 고객 | 상태 |
|----|------|------|------|
| ORD-20260508-0001 | S0001 | 삼성전자 | reserved |
| ORD-20260508-0002 | S0002 | SK하이닉스 | reserved |
| ORD-20260508-0003 | S0003 | LG이노텍 | confirmed |
| ORD-20260508-0004 | S0005 | DB하이텍 | producing |
| ORD-20260508-0005 | S0004 | 한국반도체 | released |
