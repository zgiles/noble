#import <Foundation/Foundation.h>
#import <IOBluetooth/IOBluetooth.h>

#include <string>
#include <vector>

#include "Noble.h"

@interface BLEManager : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate> {
  dispatch_queue_t _dispatchQueue;
  Noble *_noble;
}

- (id)initWithNoble:(Noble *)noble;
- (void)startScanningForServices:(std::vector<std::string>)services allowDuplicates:(bool)allowDuplicates;
- (void)stopScanning;
- (void)connectPeripheral:(std::string)uuid;
- (void)disconnectPeripheral:(std::string)uuid;
- (void)updatePeripheralRssi:(std::string)uuid;
- (void)discoverPeripheral:(std::string)uuid services:(std::vector<std::string>)services;

@end

@interface BLEManager ()

@property (nonatomic, retain) CBCentralManager *centralManager;
@property (nonatomic, retain) NSMutableDictionary *peripherals;

@end