#include "app/single_order_execution_event_handler.h"
#include "ccapi_cpp/ccapi_session.h"
namespace ccapi {
AppLogger appLogger;
AppLogger* AppLogger::logger = &appLogger;
CcapiLogger ccapiLogger;
Logger* Logger::logger = &ccapiLogger;
} /* namespace ccapi */
using ::ccapi::AppLogger;
using ::ccapi::CcapiLogger;
using ::ccapi::Element;
using ::ccapi::Event;
using ::ccapi::Logger;
using ::ccapi::Message;
using ::ccapi::Queue;
using ::ccapi::Request;
using ::ccapi::Session;
using ::ccapi::SessionConfigs;
using ::ccapi::SessionOptions;
using ::ccapi::SingleOrderExecutionEventHandler;
using ::ccapi::Subscription;
using ::ccapi::UtilString;
using ::ccapi::UtilSystem;
using ::ccapi::UtilTime;
int main(int argc, char** argv) {
  std::string exchange = UtilSystem::getEnvAsString("EXCHANGE");
  std::string instrumentRest = UtilSystem::getEnvAsString("INSTRUMENT");
  std::string instrumentWebsocket = instrumentRest;
  SingleOrderExecutionEventHandler eventHandler;
  eventHandler.exchange = exchange;
  eventHandler.instrumentRest = instrumentRest;
  eventHandler.instrumentWebsocket = instrumentWebsocket;
  eventHandler.privateDataDirectory = UtilSystem::getEnvAsString("PRIVATE_DATA_DIRECTORY");
  eventHandler.privateDataFilePrefix = UtilSystem::getEnvAsString("PRIVATE_DATA_FILE_PREFIX");
  eventHandler.privateDataFileSuffix = UtilSystem::getEnvAsString("PRIVATE_DATA_FILE_SUFFIX");
  eventHandler.privateDataOnlySaveFinalSummary = UtilString::toLower(UtilSystem::getEnvAsString("PRIVATE_DATA_ONLY_SAVE_FINAL_SUMMARY")) == "true";
  eventHandler.clockStepSeconds = UtilSystem::getEnvAsInt("CLOCK_STEP_SECONDS", 1);
  eventHandler.baseAsset = UtilSystem::getEnvAsString("BASE_ASSET_OVERRIDE");
  eventHandler.quoteAsset = UtilSystem::getEnvAsString("QUOTE_ASSET_OVERRIDE");
  eventHandler.orderPriceIncrement = UtilString::normalizeDecimalString(UtilSystem::getEnvAsString("ORDER_PRICE_INCREMENT_OVERRIDE"));
  eventHandler.orderQuantityIncrement = UtilString::normalizeDecimalString(UtilSystem::getEnvAsString("ORDER_QUANTITY_INCREMENT_OVERRIDE"));
  std::string startTimeStr = UtilSystem::getEnvAsString("START_TIME");
  eventHandler.startTimeTp = startTimeStr.empty() ? UtilTime::now() : UtilTime::parse(startTimeStr);
  eventHandler.totalDurationSeconds = UtilSystem::getEnvAsInt("TOTAL_DURATION_SECONDS");
  eventHandler.orderSide = UtilString::toUpper(UtilSystem::getEnvAsString("ORDER_SIDE"));
  eventHandler.totalTargetQuantity = UtilSystem::getEnvAsDouble("TOTAL_TARGET_QUANTITY");
  eventHandler.totalTargetQuantityInQuote = UtilSystem::getEnvAsDouble("TOTAL_TARGET_QUANTITY_IN_QUOTE");
  eventHandler.orderPriceLimit = UtilSystem::getEnvAsDouble("ORDER_PRICE_LIMIT");
  eventHandler.orderPriceLimitRelativeToMidPrice = UtilSystem::getEnvAsDouble("ORDER_PRICE_LIMIT_RELATIVE_TO_MID_PRICE");
  eventHandler.orderQuantityLimitRelativeToTarget = UtilSystem::getEnvAsDouble("ORDER_QUANTITY_LIMIT_RELATIVE_TO_TARGET");
  eventHandler.twapOrderQuantityRandomizationMax = UtilSystem::getEnvAsDouble("TWAP_ORDER_QUANTITY_RANDOMIZATION_MAX");
  eventHandler.povOrderQuantityParticipationRate = UtilSystem::getEnvAsDouble("POV_ORDER_QUANTITY_PARTICIPATION_RATE");
  eventHandler.isKapa = UtilSystem::getEnvAsDouble("IS_URGENCY");
  std::string tradingMode = UtilSystem::getEnvAsString("TRADING_MODE");
  APP_LOGGER_INFO("******** Trading mode is " + tradingMode + "! ********");
  if (tradingMode == "paper") {
    eventHandler.tradingMode = SingleOrderExecutionEventHandler::TradingMode::PAPER;
  } else if (tradingMode == "backtest") {
    eventHandler.tradingMode = SingleOrderExecutionEventHandler::TradingMode::BACKTEST;
  }
  if (eventHandler.tradingMode == SingleOrderExecutionEventHandler::TradingMode::PAPER ||
      eventHandler.tradingMode == SingleOrderExecutionEventHandler::TradingMode::BACKTEST) {
    eventHandler.makerFee = UtilSystem::getEnvAsDouble("MAKER_FEE");
    eventHandler.makerBuyerFeeAsset = UtilSystem::getEnvAsString("MAKER_BUYER_FEE_ASSET");
    eventHandler.makerSellerFeeAsset = UtilSystem::getEnvAsString("MAKER_SELLER_FEE_ASSET");
    eventHandler.takerFee = UtilSystem::getEnvAsDouble("TAKER_FEE");
    eventHandler.takerBuyerFeeAsset = UtilSystem::getEnvAsString("TAKER_BUYER_FEE_ASSET");
    eventHandler.takerSellerFeeAsset = UtilSystem::getEnvAsString("TAKER_SELLER_FEE_ASSET");
    eventHandler.baseBalance = UtilSystem::getEnvAsDouble("INITIAL_BASE_BALANCE") * eventHandler.baseAvailableBalanceProportion;
    eventHandler.quoteBalance = UtilSystem::getEnvAsDouble("INITIAL_QUOTE_BALANCE") * eventHandler.quoteAvailableBalanceProportion;
  }
  if (eventHandler.tradingMode == SingleOrderExecutionEventHandler::TradingMode::BACKTEST) {
    eventHandler.historicalMarketDataDirectory = UtilSystem::getEnvAsString("HISTORICAL_MARKET_DATA_DIRECTORY");
    eventHandler.historicalMarketDataFilePrefix = UtilSystem::getEnvAsString("HISTORICAL_MARKET_DATA_FILE_PREFIX");
    eventHandler.historicalMarketDataFileSuffix = UtilSystem::getEnvAsString("HISTORICAL_MARKET_DATA_FILE_SUFFIX");
  }
  std::string tradingStrategy = UtilSystem::getEnvAsString("TRADING_STRATEGY");
  APP_LOGGER_INFO("******** Trading strategy is " + tradingStrategy + "! ********");
  if (tradingStrategy == "twap") {
    eventHandler.tradingStrategy = SingleOrderExecutionEventHandler::TradingStrategy::TWAP;
  } else if (tradingStrategy == "vwap") {
    eventHandler.tradingStrategy = SingleOrderExecutionEventHandler::TradingStrategy::VWAP;
  } else if (tradingStrategy == "pov") {
    eventHandler.tradingStrategy = SingleOrderExecutionEventHandler::TradingStrategy::POV;
  } else if (tradingStrategy == "is") {
    eventHandler.tradingStrategy = SingleOrderExecutionEventHandler::TradingStrategy::IS;
  }
  std::set<std::string> useGetAccountsToGetAccountBalancesExchangeSet{"coinbase", "kucoin"};
  if (useGetAccountsToGetAccountBalancesExchangeSet.find(eventHandler.exchange) != useGetAccountsToGetAccountBalancesExchangeSet.end()) {
    eventHandler.useGetAccountsToGetAccountBalances = true;
  }
  std::shared_ptr<std::promise<void>> promisePtr(new std::promise<void>());
  eventHandler.promisePtr = promisePtr;
  SessionOptions sessionOptions;
  sessionOptions.httpConnectionPoolIdleTimeoutMilliSeconds = 1 + eventHandler.accountBalanceRefreshWaitSeconds;
  SessionConfigs sessionConfigs;
  Session session(sessionOptions, sessionConfigs, &eventHandler);
  if (exchange == "kraken") {
    Request request(Request::Operation::GENERIC_PUBLIC_REQUEST, "kraken", "", "Get Instrument Symbol For Websocket");
    request.appendParam({
        {"HTTP_METHOD", "GET"},
        {"HTTP_PATH", "/0/public/AssetPairs"},
        {"HTTP_QUERY_STRING", "pair=" + instrumentRest},
    });
    Queue<Event> eventQueue;
    session.sendRequest(request, &eventQueue);
    std::vector<Event> eventList = eventQueue.purge();
    for (const auto& event : eventList) {
      if (event.getType() == Event::Type::RESPONSE) {
        rj::Document document;
        document.Parse(event.getMessageList().at(0).getElementList().at(0).getValue("HTTP_BODY").c_str());
        eventHandler.instrumentWebsocket = document["result"][instrumentRest.c_str()]["wsname"].GetString();
        break;
      }
    }
  } else if (exchange.rfind("binance", 0) == 0) {
    eventHandler.instrumentWebsocket = UtilString::toLower(instrumentRest);
  }
  Request request(Request::Operation::GET_INSTRUMENT, eventHandler.exchange, eventHandler.instrumentRest, "GET_INSTRUMENT");
  if (eventHandler.tradingMode == SingleOrderExecutionEventHandler::TradingMode::BACKTEST && !eventHandler.baseAsset.empty() &&
      !eventHandler.quoteAsset.empty() && !eventHandler.orderPriceIncrement.empty() && !eventHandler.orderQuantityIncrement.empty()) {
    Event virtualEvent;
    Message message;
    message.setTime(eventHandler.startDateTp);
    message.setTimeReceived(eventHandler.startDateTp);
    message.setCorrelationIdList({request.getCorrelationId()});
    std::vector<Element> elementList;
    virtualEvent.setType(Event::Type::RESPONSE);
    message.setType(Message::Type::GET_INSTRUMENT);
    Element element;
    element.insert("BASE_ASSET", eventHandler.baseAsset);
    element.insert("QUOTE_ASSET", eventHandler.quoteAsset);
    element.insert("PRICE_INCREMENT", eventHandler.orderPriceIncrement);
    element.insert("QUANTITY_INCREMENT", eventHandler.orderQuantityIncrement);
    elementList.emplace_back(std::move(element));
    message.setElementList(elementList);
    virtualEvent.setMessageList({message});
    eventHandler.processEvent(virtualEvent, &session);
  } else {
    session.sendRequest(request);
  }
  promisePtr->get_future().wait();
  session.stop();
  return EXIT_SUCCESS;
}