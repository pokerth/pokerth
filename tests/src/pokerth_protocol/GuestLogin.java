
package pokerth_protocol;
//
// This file was generated by the BinaryNotes compiler.
// See http://bnotes.sourceforge.net 
// Any modifications to this file will be lost upon recompilation of the source ASN.1. 
//

import org.bn.*;
import org.bn.annotations.*;
import org.bn.annotations.constraints.*;
import org.bn.coders.*;
import org.bn.types.*;




    @ASN1PreparedElement
    @ASN1Sequence ( name = "GuestLogin", isSet = false )
    public class GuestLogin implements IASN1PreparedElement {
            
    @ASN1String( name = "", 
        stringType = UniversalTag.UTF8String , isUCS = false )
    @ASN1ValueRangeConstraint ( 
		
		min = 1L, 
		
		max = 64L 
		
	   )
	   
        @ASN1Element ( name = "nickName", isOptional =  false , hasTag =  false  , hasDefaultValue =  false  )
    
	private String nickName = null;
                
  
        
        public String getNickName () {
            return this.nickName;
        }

        

        public void setNickName (String value) {
            this.nickName = value;
        }
        
  
                    
        
        public void initWithDefaults() {
            
        }

        private static IASN1PreparedElementData preparedData = CoderFactory.getInstance().newPreparedElementData(GuestLogin.class);
        public IASN1PreparedElementData getPreparedData() {
            return preparedData;
        }

            
    }
            