<script lang="ts">

    import { onMount } from 'svelte'
    import type { IAppManagement, IWebsocketMessageListener } from '../utils/interfaces'
    import type { ResponseWrapper } from '../generated/flatbuffers/webmanager'

    export var app: IAppManagement
    export var lastMessage = ''
    export const M = new (class implements IWebsocketMessageListener {
        onMessage(messageWrapper: ResponseWrapper): void {
            //lastMessage = messageWrapper
        }
    })()

    onMount(() => {
        return app.registerWebsocketMessageTypes(M, 1, 2, 3)
    })
</script>

<h1>I am Sensact!</h1>
<p>{lastMessage}</p>
