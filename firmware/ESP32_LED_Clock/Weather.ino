// ==================== 天气功能（从借鉴代码 Weather.ino 和 Weather3day.ino 整合） ====================

// 获取实时天气（和风天气 API）
void getWeather() {
  HTTPClient http;
  String url = "https://devapi.qweather.com/v7/weather/now?location=" + city + "&key=" + zx_key;
  Serial.print(url);
  http.begin(url.c_str());
  String payload;
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.println(payload);

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    const char* now_temp = doc["now"]["temp"];
    const char* now_icon = doc["now"]["icon"];
    const char* now_humidity = doc["now"]["humidity"];

    strcpy(wea_temp1, now_temp);
    wea_code = atoi(now_icon);
    wea_hm = atoi(now_humidity);

    Serial.print("temp:");
    Serial.println(wea_temp1);
    Serial.print("code:");
    Serial.println(wea_code);
    Serial.print("humidity:");
    Serial.println(wea_hm);
  }
}

// 获取三天天气预报
void get3DayWeather() {
  HTTPClient http;
  String url = "https://devapi.qweather.com/v7/weather/3d?location=" + city + "&key=" + zx_key;
  Serial.print(url);
  http.begin(url.c_str());
  String payload;
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.println(payload);

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    // 第一天
    const char* day1_tempMin = doc["daily"][0]["tempMin"];
    const char* day1_tempMax = doc["daily"][0]["tempMax"];
    const char* day1_iconDay = doc["daily"][0]["iconDay"];
    const char* day1_iconNight = doc["daily"][0]["iconNight"];
    const char* day1_fxDate = doc["daily"][0]["fxDate"];

    strcpy(tem_day1_min, day1_tempMin);
    strcpy(tem_day1_max, day1_tempMax);
    wea_code_day1 = atoi(day1_iconDay);
    wea_code_night1 = atoi(day1_iconNight);
    strcpy(day1_date, day1_fxDate);

    // 第二天
    const char* day2_tempMin = doc["daily"][1]["tempMin"];
    const char* day2_tempMax = doc["daily"][1]["tempMax"];
    const char* day2_iconDay = doc["daily"][1]["iconDay"];
    const char* day2_iconNight = doc["daily"][1]["iconNight"];
    const char* day2_fxDate = doc["daily"][1]["fxDate"];

    strcpy(tem_day2_min, day2_tempMin);
    strcpy(tem_day2_max, day2_tempMax);
    wea_code_day2 = atoi(day2_iconDay);
    wea_code_night2 = atoi(day2_iconNight);
    strcpy(day2_date, day2_fxDate);

    // 第三天
    const char* day3_tempMin = doc["daily"][2]["tempMin"];
    const char* day3_tempMax = doc["daily"][2]["tempMax"];
    const char* day3_iconDay = doc["daily"][2]["iconDay"];
    const char* day3_iconNight = doc["daily"][2]["iconNight"];
    const char* day3_fxDate = doc["daily"][2]["fxDate"];

    strcpy(tem_day3_min, day3_tempMin);
    strcpy(tem_day3_max, day3_tempMax);
    wea_code_day3 = atoi(day3_iconDay);
    wea_code_night3 = atoi(day3_iconNight);
    strcpy(day3_date, day3_fxDate);
  }
}

// 显示三天天气预报
void show3dayWeather() {
  // 第一天
  showTQ(wea_code_day1, 64, 0);
  drawChars(70, 0, tem_day1_min, color4);
  drawSmBit(78, -2, wd, 3, 8, color4);
  drawChars(70, 8, tem_day1_max, color4);
  drawSmBit(78, 6, wd, 3, 8, color4);

  // 第二天
  showTQ(wea_code_day2, 96, 0);
  drawChars(102, 0, tem_day2_min, color4);
  drawSmBit(110, -2, wd, 3, 8, color4);
  drawChars(102, 8, tem_day2_max, color4);
  drawSmBit(110, 6, wd, 3, 8, color4);

  // 第三天
  showTQ(wea_code_day3, 64, 16);
  drawChars(70, 16, tem_day3_min, color4);
  drawSmBit(78, 14, wd, 3, 8, color4);
  drawChars(70, 24, tem_day3_max, color4);
  drawSmBit(78, 22, wd, 3, 8, color4);
}

// 显示天气图标
void showTQ(int code, int x, int y) {
  switch (code) {
    case 100: case 900: case 901: case 902: case 903: case 904: case 905: case 906:
      drawColorBit(x, y, tianqiqing, 16, 16);
      break;
    case 101: case 102: case 103: case 104: case 105: case 106: case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114: case 115: case 116: case 117: case 118: case 119: case 120: case 121: case 122: case 123: case 124: case 125: case 126: case 127: case 128: case 129: case 130: case 131: case 132: case 133: case 134: case 135: case 136: case 137: case 138: case 139: case 140: case 141: case 142: case 143: case 144: case 145: case 146: case 147: case 148: case 149: case 150: case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159: case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167: case 168: case 169: case 170: case 171: case 172: case 173: case 174: case 175: case 176: case 177: case 178: case 179: case 180: case 181: case 182: case 183: case 184: case 185: case 186: case 187: case 188: case 189: case 190: case 191: case 192: case 193: case 194: case 195: case 196: case 197: case 198: case 199:
      drawColorBit(x, y, tianqiyin, 16, 16);
      break;
    case 300: case 301: case 302: case 303: case 304: case 305: case 306: case 307: case 308: case 309: case 310: case 311: case 312: case 313: case 314: case 315: case 316: case 317: case 318: case 319: case 320: case 321: case 322: case 323: case 324: case 325: case 326: case 327: case 328: case 329: case 330: case 331: case 332: case 333: case 334: case 335: case 336: case 337: case 338: case 339: case 340: case 341: case 342: case 343: case 344: case 345: case 346: case 347: case 348: case 349: case 350: case 351: case 352: case 353: case 354: case 355: case 356: case 357: case 358: case 359: case 360: case 361: case 362: case 363: case 364: case 365: case 366: case 367: case 368: case 369: case 370: case 371: case 372: case 373: case 374: case 375: case 376: case 377: case 378: case 379: case 380: case 381: case 382: case 383: case 384: case 385: case 386: case 387: case 388: case 389: case 390: case 391: case 392: case 393: case 394: case 395: case 396: case 397: case 398: case 399:
      drawColorBit(x, y, tianqiyu, 16, 16);
      break;
    case 400: case 401: case 402: case 403: case 404: case 405: case 406: case 407: case 408: case 409: case 410: case 411: case 412: case 413: case 414: case 415: case 416: case 417: case 418: case 419: case 420: case 421: case 422: case 423: case 424: case 425: case 426: case 427: case 428: case 429: case 430: case 431: case 432: case 433: case 434: case 435: case 436: case 437: case 438: case 439: case 440: case 441: case 442: case 443: case 444: case 445: case 446: case 447: case 448: case 449: case 450: case 451: case 452: case 453: case 454: case 455: case 456: case 457: case 458: case 459: case 460: case 461: case 462: case 463: case 464: case 465: case 466: case 467: case 468: case 469: case 470: case 471: case 472: case 473: case 474: case 475: case 476: case 477: case 478: case 479: case 480: case 481: case 482: case 483: case 484: case 485: case 486: case 487: case 488: case 489: case 490: case 491: case 492: case 493: case 494: case 495: case 496: case 497: case 498: case 499:
      drawColorBit(x, y, tianqixue, 16, 16);
      break;
    case 500: case 501: case 502: case 503: case 504: case 505: case 506: case 507: case 508: case 509: case 510: case 511: case 512: case 513: case 514: case 515: case 516: case 517: case 518: case 519: case 520: case 521: case 522: case 523: case 524: case 525: case 526: case 527: case 528: case 529: case 530: case 531: case 532: case 533: case 534: case 535: case 536: case 537: case 538: case 539: case 540: case 541: case 542: case 543: case 544: case 545: case 546: case 547: case 548: case 549: case 550: case 551: case 552: case 553: case 554: case 555: case 556: case 557: case 558: case 559: case 560: case 561: case 562: case 563: case 564: case 565: case 566: case 567: case 568: case 569: case 570: case 571: case 572: case 573: case 574: case 575: case 576: case 577: case 578: case 579: case 580: case 581: case 582: case 583: case 584: case 585: case 586: case 587: case 588: case 589: case 590: case 591: case 592: case 593: case 594: case 595: case 596: case 597: case 598: case 599:
      drawColorBit(x, y, tianqiwu, 16, 16);
      break;
    default:
      drawColorBit(x, y, tianqiqing, 16, 16);
      break;
  }
}
